#include "LeeGameplayAbility_Dodge.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "TimerManager.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Math/RotationMatrix.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"

// 생성자 — 태그/그룹/정책 설정
ULeeGameplayAbility_Dodge::ULeeGameplayAbility_Dodge(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationPolicy   = ELeeAbilityActivationPolicy::OnInputTriggered;
	ActivationGroup    = ELeeAbilityActivationGroup::Exclusive_Replaceable;

	AbilityTags.AddTag(MyTags::Souls::Ability_Dodge);

	// 회피 동작 중 상태 태그 + 스태미나 회복 정지 (종료 시 자동 제거)
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Dodge_Active);
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Stamina_RegenBlocked);

	// 그로기 중 회피 불가
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Groggy);

	// 가드 중 회피 입력 → 가드를 캔슬하고 회피 (AbilityTags = Souls.Abilities.Guard 매칭)
	CancelAbilitiesWithTag.AddTag(MyTags::Souls::Ability_Guard);
}

// 활성화 — 몽타주 재생 + 서버 i-frame 타이머 예약 + 퍼펙트 회피 이벤트 대기
void ULeeGameplayAbility_Dodge::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Cost GE(스태미나 소모)는 BP에서 지정 — 부족하면 여기서 실패
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!DodgeMontage)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeGA_Dodge] DodgeMontage가 설정되지 않음. BP에서 지정해주세요."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 몽타주 재생
	const FName DodgeSection = SelectDodgeSection();
	if (DodgeMontage->GetSectionIndex(DodgeSection) == INDEX_NONE)
	{
		UE_LOG(LogLee, Error, TEXT("[LeeGA_Dodge] DodgeMontage [%s]에 섹션 [%s]이 없습니다. BP의 섹션 이름을 확인해주세요."),
			*GetNameSafe(DodgeMontage), *DodgeSection.ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, DodgeMontage, /*PlayRate*/1.0f, DodgeSection);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();

	// 2. [서버] i-frame 시작/종료 타이머 — AnimNotify에 의존하지 않아 데디 서버에서도 보장
	if (ActorInfo->IsNetAuthority())
	{
		FTimerManager& TimerManager = GetWorld()->GetTimerManager();
		TimerManager.SetTimer(IFrameBeginTimerHandle,
			FTimerDelegate::CreateUObject(this, &ThisClass::OnIFrameBegin), IFrameStartTime, /*bLoop*/false);
		TimerManager.SetTimer(IFrameEndTimerHandle,
			FTimerDelegate::CreateUObject(this, &ThisClass::OnIFrameEnd), IFrameEndTime, /*bLoop*/false);
	}

	// 3. 퍼펙트 회피 판정 이벤트 대기 (DefenseComponent가 발송 )
	UAbilityTask_WaitGameplayEvent* PerfectDodgeTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, MyTags::Souls::Event_Defense_PerfectDodge,
			/*OptionalExternalOwner*/nullptr, /*OnlyTriggerOnce*/true, /*OnlyMatchExact*/true);
	PerfectDodgeTask->EventReceived.AddDynamic(this, &ThisClass::OnPerfectDodgeEventReceived);
	PerfectDodgeTask->ReadyForActivation();
}

// 종료 — 무적/퍼펙트 윈도우 GE와 타이머를 무조건 정리 (태그 누수 방지 안전장치)
void ULeeGameplayAbility_Dodge::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	CleanupDefenseWindows();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// [서버] i-frame 시작 — 무적 GE(핸들 보관) + 퍼펙트 회피 윈도우 GE 적용
void ULeeGameplayAbility_Dodge::OnIFrameBegin()
{
	if (InvincibleEffect)
	{
		const FGameplayEffectSpecHandle InvincibleSpec =
			MakeOutgoingGameplayEffectSpec(InvincibleEffect, GetAbilityLevel());
		InvincibleHandle = ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, InvincibleSpec);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_Dodge] InvincibleEffect가 설정되지 않음. BP에서 GE_DodgeInvincible을 지정해주세요."));
	}

	// 퍼펙트 회피 윈도우 — Duration GE 만료로 자동 제거, EndAbility에서도 이중 정리
	PerfectDodgeWindowHandle = ApplyDurationEffect(PerfectDodgeWindowEffect, PerfectDodgeWindow);
}

// [서버] i-frame 종료 — 무적 GE 핸들 제거 (LooseTag 제거 금지 — Finisher 무적과 태그 공유)
void ULeeGameplayAbility_Dodge::OnIFrameEnd()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && InvincibleHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(InvincibleHandle);
	}
	InvincibleHandle.Invalidate();
}

// 퍼펙트 회피 성공 — 반격 윈도우 GE + 잔상 GameplayCue (서버에서 실행 → 전 클라이언트 연출)
void ULeeGameplayAbility_Dodge::OnPerfectDodgeEventReceived(FGameplayEventData /*Payload*/)
{
	// [임시 디버그] 퍼펙트 회피 이벤트 수신 확인 (#2)
	//  - 이 로그가 "안 뜨는데" 회피 성공처럼 보였다면 → 판정(윈도우) 문제 = PerfectDodgeWindow 타이밍
	//  - 이 로그는 "뜨는데" 잔상 연출이 없으면 → Cue 전달/연출 문제(멀티캐스트 드롭 또는 잔상 로직)
	const AActor* DbgAvatar = GetAvatarActorFromActorInfo();
	UE_LOG(LogLee, Warning, TEXT("[임시디버그][PerfectDodge] 이벤트 수신 → Cue 실행 시도 NetMode=%d"),
		DbgAvatar ? (int32)DbgAvatar->GetNetMode() : -1);

	ApplyDurationEffect(CounterWindowEffect, CounterWindowDuration);

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = GetAvatarActorFromActorInfo();
		ASC->ExecuteGameplayCue(MyTags::Souls::GameplayCue_Dodge_Perfect, CueParams);
	}
}

// 몽타주 정상 완료
void ULeeGameplayAbility_Dodge::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}

// 몽타주 인터럽트/외부 취소 — EndAbility가 무적 해제까지 정리
void ULeeGameplayAbility_Dodge::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/true);
}

// 무적/퍼펙트 윈도우 GE 핸들 제거 + 타이머 해제 — 모든 종료 경로가 여기를 거친다
void ULeeGameplayAbility_Dodge::CleanupDefenseWindows()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IFrameBeginTimerHandle);
		World->GetTimerManager().ClearTimer(IFrameEndTimerHandle);
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		if (InvincibleHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(InvincibleHandle);
		}
		if (PerfectDodgeWindowHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(PerfectDodgeWindowHandle);
		}
	}
	InvincibleHandle.Invalidate();
	PerfectDodgeWindowHandle.Invalidate();
}

// SetByCaller Duration 방식 GE 적용 헬퍼 — BP 애셋의 Duration이 SetByCaller(Souls.SetByCaller.Duration)여야 한다
FVector ULeeGameplayAbility_Dodge::GetDodgeInputVector() const
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn)
	{
		return FVector::ZeroVector;
	}

	// 같은 프레임의 WASD 입력을 우선 사용하고, 이미 소비되었다면
	// 직전 입력 또는 현재 속도를 보조 값으로 사용한다.
	FVector InputVector = Pawn->GetPendingMovementInputVector();
	if (InputVector.IsNearlyZero())
	{
		InputVector = Pawn->GetLastMovementInputVector();
	}
	if (InputVector.IsNearlyZero())
	{
		InputVector = Pawn->GetVelocity();
	}

	InputVector.Z = 0.0f;
	return InputVector;
}

FName ULeeGameplayAbility_Dodge::SelectDodgeSection() const
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn)
	{
		return DodgeForwardSection;
	}

	const FVector InputVector = GetDodgeInputVector();
	if (InputVector.IsNearlyZero())
	{
		return DodgeForwardSection;
	}

	FRotator ReferenceRotation = Pawn->GetActorRotation();
	if (const AController* Controller = Pawn->GetController())
	{
		ReferenceRotation = Controller->GetControlRotation();
	}
	ReferenceRotation.Pitch = 0.0f;
	ReferenceRotation.Roll = 0.0f;

	const FRotationMatrix ReferenceMatrix(ReferenceRotation);
	const FVector Forward = ReferenceMatrix.GetUnitAxis(EAxis::X);
	const FVector Right = ReferenceMatrix.GetUnitAxis(EAxis::Y);
	const float ForwardAmount = FVector::DotProduct(InputVector, Forward);
	const float RightAmount = FVector::DotProduct(InputVector, Right);

	if (FMath::Abs(ForwardAmount) >= FMath::Abs(RightAmount))
	{
		return ForwardAmount >= 0.0f ? DodgeForwardSection : DodgeBackwardSection;
	}

	return RightAmount >= 0.0f ? DodgeRightSection : DodgeLeftSection;
}

FActiveGameplayEffectHandle ULeeGameplayAbility_Dodge::ApplyDurationEffect(
	TSubclassOf<UGameplayEffect> EffectClass, float Duration)
{
	if (!EffectClass || Duration <= 0.0f)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(EffectClass, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Duration, Duration);
	return ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
}
