// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeDeathScreenWidget.h"

#include "CommonActivatableWidget.h"
#include "CommonUIExtensions.h"
#include "Components/Image.h"
#include "TimerManager.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AMessage/LeeVerbMessage.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

// 사망/부활 메시지 채널 구독 + 화면을 숨김 상태로 초기화
void ULeeDeathScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogLee, Log, TEXT("[DeathScreen] Construct %s World=%s"), *GetName(), *GetNameSafe(GetWorld()));

	// 26.11.04 추가 - HUD 레이어 스택과 확장 슬롯은 위젯을 풀에 넣었다가 같은 인스턴스를 다시 붙인다.
	// 이전 맵에서 켜 둔 YOU DIED / Clear 상태가 그대로 남지 않도록, 붙을 때마다 숨김 상태로 되돌린다
	// (WB_YouDied는 모든 요소가 루트 Canvas 아래에 있고, 루트 기본값이 Collapsed → 루트만 숨기면 처음 상태와 같다)
	StopAllAnimations();
	SetDimBackgroundVisible(false);
	if (UWidget* Root = GetRootWidget())
	{
		Root->SetVisibility(ESlateVisibility::Collapsed);
	}

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	YouDiedListenerHandle = MessageSystem.RegisterListener(
		MyTags::Souls::Message_Death_YouDied, this, &ThisClass::HandleYouDiedMessage);
	RespawnListenerHandle = MessageSystem.RegisterListener(
		MyTags::Souls::Message_Death_Respawn, this, &ThisClass::HandleRespawnMessage);
	PartyWipedListenerHandle = MessageSystem.RegisterListener(
		MyTags::Souls::Message_Death_PartyWiped, this, &ThisClass::HandlePartyWipedMessage);
	BossVictoryListenerHandle = MessageSystem.RegisterListener(
		MyTags::Souls::Message_Boss_Victory, this, &ThisClass::HandleBossVictoryMessage);
}

// 구독 해제 + 부활 질문 위젯 정리
void ULeeDeathScreenWidget::NativeDestruct()
{
	UE_LOG(LogLee, Log, TEXT("[DeathScreen] Destruct %s World=%s"), *GetName(), *GetNameSafe(GetWorld()));

	YouDiedListenerHandle.Unregister();
	RespawnListenerHandle.Unregister();
	PartyWipedListenerHandle.Unregister();
	BossVictoryListenerHandle.Unregister();
	HideRespawnPrompt();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(YouDiedScreenTimerHandle);	// 26.11.12 추가
	}

	Super::NativeDestruct();
}

// 사망 메시지 수신 → (26.11.12 변경) 사망 몽타주가 끝난 뒤 YOU DIED, 그 뒤 RespawnPromptDelay에 부활 질문
void ULeeDeathScreenWidget::HandleYouDiedMessage(FGameplayTag Channel, const FLeeVerbMessage& Message)
{
	UE_LOG(LogLee, Log, TEXT("[DeathScreen] YouDied 수신 %s World=%s"), *GetName(), *GetNameSafe(GetWorld()));
	ScheduleYouDiedScreen(/*bWithRespawnPrompt*/ true);
}

// 전멸 메시지 수신 → (26.11.12 변경) 사망 몽타주가 끝난 뒤 YOU DIED 표시만 (부활 질문 없음)
void ULeeDeathScreenWidget::HandlePartyWipedMessage(FGameplayTag Channel, const FLeeVerbMessage& Message)
{
	UE_LOG(LogLee, Log, TEXT("[DeathScreen] PartyWiped 수신 %s World=%s"), *GetName(), *GetNameSafe(GetWorld()));
	ScheduleYouDiedScreen(/*bWithRespawnPrompt*/ false);
}

// 26.11.12 추가 - 남은 몽타주 시간(최대 MaxDeathMontageWait) + DelayAfterDeathMontage 뒤에 YOU DIED를 띄운다. 0이면 바로
void ULeeDeathScreenWidget::ScheduleYouDiedScreen(bool bWithRespawnPrompt)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(YouDiedScreenTimerHandle);

	const float Delay = FMath::Min(GetRemainingDeathMontageTime(), MaxDeathMontageWait) + DelayAfterDeathMontage;
	UE_LOG(LogLee, Log, TEXT("[DeathScreen] 사망 몽타주 대기 %.2f초 후 YOU DIED 표시"), Delay);

	if (Delay <= KINDA_SMALL_NUMBER)
	{
		ShowYouDiedScreen(bWithRespawnPrompt);
		return;
	}

	World->GetTimerManager().SetTimer(YouDiedScreenTimerHandle,
		FTimerDelegate::CreateUObject(this, &ThisClass::ShowYouDiedScreen, bWithRespawnPrompt), Delay, false);
}

// 26.11.12 추가 - 어두운 배경 + BP OnYouDied, 개인 사망이면 RespawnPromptDelay 뒤 부활 질문
void ULeeDeathScreenWidget::ShowYouDiedScreen(bool bWithRespawnPrompt)
{
	SetDimBackgroundVisible(true);
	OnYouDied();

	if (bWithRespawnPrompt)
	{
		GetWorld()->GetTimerManager().SetTimer(
			RespawnPromptTimerHandle, this, &ThisClass::ShowRespawnPrompt, RespawnPromptDelay, false);
	}
}

// 26.11.12 추가 - 내 캐릭터 메시의 AnimInstance에서 재생 중인 몽타주의 남은 재생 시간 (사망 직후라 사망 몽타주가 재생 중)
// AM_Death는 Auto Blend Out이 꺼져 있어 끝나도 마지막 프레임에 머무르므로, "끝" 콜백 대신 길이 - 현재 위치로 계산한다
float ULeeDeathScreenWidget::GetRemainingDeathMontageTime() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwningPlayerPawn());
	const UAnimInstance* AnimInstance = (Character && Character->GetMesh()) ? Character->GetMesh()->GetAnimInstance() : nullptr;
	const FAnimMontageInstance* MontageInstance = AnimInstance ? AnimInstance->GetActiveMontageInstance() : nullptr;
	if (!MontageInstance || !MontageInstance->Montage)
	{
		return 0.0f;
	}

	const float PlayRate = FMath::Max(FMath::Abs(MontageInstance->GetPlayRate()), KINDA_SMALL_NUMBER);
	return FMath::Max(0.0f, (MontageInstance->Montage->GetPlayLength() - MontageInstance->GetPosition()) / PlayRate);
}

// 26.11.02 추가 - 보스 처치 메시지 수신 → BP OnBossVictory (26.11.03 - 승리 화면은 어두운 배경 없음)
void ULeeDeathScreenWidget::HandleBossVictoryMessage(FGameplayTag Channel, const FLeeVerbMessage& Message)
{
	UE_LOG(LogLee, Log, TEXT("[DeathScreen] BossVictory 수신 %s World=%s"), *GetName(), *GetNameSafe(GetWorld()));
	GetWorld()->GetTimerManager().ClearTimer(YouDiedScreenTimerHandle);	// 26.11.12 추가 - 대기 중이던 YOU DIED 취소
	SetDimBackgroundVisible(false);
	OnBossVictory();
}

// 26.11.03 추가 - 어두운 배경 표시/숨김 (표시 중에도 클릭은 막지 않도록 HitTestInvisible)
void ULeeDeathScreenWidget::SetDimBackgroundVisible(bool bVisible)
{
	if (DimBackground)
	{
		DimBackground->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

// 부활 메시지 수신 → 부활 질문 닫기 + BP OnRespawn
void ULeeDeathScreenWidget::HandleRespawnMessage(FGameplayTag Channel, const FLeeVerbMessage& Message)
{
	UE_LOG(LogLee, Log, TEXT("[DeathScreen] Respawn 수신 %s World=%s"), *GetName(), *GetNameSafe(GetWorld()));
	HideRespawnPrompt();
	GetWorld()->GetTimerManager().ClearTimer(YouDiedScreenTimerHandle);	// 26.11.12 추가 - 몽타주 대기 중 부활하면 YOU DIED를 띄우지 않음
	OnRespawn();
}

// 부활 질문 위젯을 Menu 레이어에 Push (CommonUI가 커서/입력 모드 전환 처리)
void ULeeDeathScreenWidget::ShowRespawnPrompt()
{
	if (!RespawnPromptClass || RespawnPromptWidget.IsValid())
	{
		return;
	}

	RespawnPromptWidget = UCommonUIExtensions::PushContentToLayer_ForPlayer(
		GetOwningLocalPlayer(), MyTags::Lyra::UI_Layer_Menu, RespawnPromptClass);
}

// 대기 중인 타이머 취소 + 떠 있는 부활 질문 위젯 제거
void ULeeDeathScreenWidget::HideRespawnPrompt()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnPromptTimerHandle);
	}

	if (UCommonActivatableWidget* Prompt = RespawnPromptWidget.Get())
	{
		UCommonUIExtensions::PopContentFromLayer(Prompt);
	}
	RespawnPromptWidget.Reset();
}
