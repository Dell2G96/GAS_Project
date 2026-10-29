// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeDeathScreenWidget.h"

#include "CommonActivatableWidget.h"
#include "CommonUIExtensions.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AMessage/LeeVerbMessage.h"

// 사망/부활 메시지 채널 구독
void ULeeDeathScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	YouDiedListenerHandle = MessageSystem.RegisterListener(
		MyTags::Souls::Message_Death_YouDied, this, &ThisClass::HandleYouDiedMessage);
	RespawnListenerHandle = MessageSystem.RegisterListener(
		MyTags::Souls::Message_Death_Respawn, this, &ThisClass::HandleRespawnMessage);
}

// 구독 해제 + 부활 질문 위젯 정리
void ULeeDeathScreenWidget::NativeDestruct()
{
	YouDiedListenerHandle.Unregister();
	RespawnListenerHandle.Unregister();
	HideRespawnPrompt();

	Super::NativeDestruct();
}

// 사망 메시지 수신 → BP OnYouDied, RespawnPromptDelay 뒤 부활 질문 표시
void ULeeDeathScreenWidget::HandleYouDiedMessage(FGameplayTag Channel, const FLeeVerbMessage& Message)
{
	OnYouDied();

	GetWorld()->GetTimerManager().SetTimer(
		RespawnPromptTimerHandle, this, &ThisClass::ShowRespawnPrompt, RespawnPromptDelay, false);
}

// 부활 메시지 수신 → 부활 질문 닫기 + BP OnRespawn
void ULeeDeathScreenWidget::HandleRespawnMessage(FGameplayTag Channel, const FLeeVerbMessage& Message)
{
	HideRespawnPrompt();
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
