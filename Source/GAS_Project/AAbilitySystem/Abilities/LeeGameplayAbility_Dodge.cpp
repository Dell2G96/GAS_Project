#include "LeeGameplayAbility_Dodge.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "TimerManager.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
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
	// 26.11.13 수정 - ServerInitiated → LocalPredicted
	// 더블탭 판정은 누른 클라이언트가 해야 하므로 클라이언트에서 즉시 활성화되고, 결정을 서버에 보낼 수 있어야 한다.
	// i-frame(무적)은 계속 서버 타이머가 담당한다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
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

	// 26.11.06 추가 - 부활 중 회피 입력 → 부활을 끊고 회피
	CancelAbilitiesWithTag.AddTag(MyTags::Souls::Ability_Revive);
}

// 활성화 — 스태미나 소모 + 퍼펙트 회피 이벤트 대기 + 스텝/구르기 결정 대기 시작
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

	bMoveStarted = false;

	// 1. 퍼펙트 회피 판정 이벤트 대기 (DefenseComponent가 발송 )
	UAbilityTask_WaitGameplayEvent* PerfectDodgeTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, MyTags::Souls::Event_Defense_PerfectDodge,
			/*OptionalExternalOwner*/nullptr, /*OnlyTriggerOnce*/true, /*OnlyMatchExact*/true);
	PerfectDodgeTask->EventReceived.AddDynamic(this, &ThisClass::OnPerfectDodgeEventReceived);
	PerfectDodgeTask->ReadyForActivation();

	// 2. 26.11.13 추가 - 스텝/구르기 결정 대기
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();

	if (ActorInfo->IsLocallyControlled())
	{
		// [로컬 조작자] 키를 한 번 뗀 뒤 다시 눌러야 두 번째 입력으로 인정한다.
		// (입력이 Triggered에 바인딩되어 있어 누르고 있는 동안 Pressed가 반복으로 들어오기 때문)
		UAbilityTask_WaitInputRelease* ReleaseTask =
			UAbilityTask_WaitInputRelease::WaitInputRelease(this, /*bTestAlreadyReleased*/true);
		ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnFirstInputReleased);
		ReleaseTask->ReadyForActivation();

		TimerManager.SetTimer(DecisionTimerHandle,
			FTimerDelegate::CreateUObject(this, &ThisClass::OnDoubleTapWindowExpired),
			FMath::Max(DoubleTapWindow, KINDA_SMALL_NUMBER), /*bLoop*/false);
	}
	else if (IsServerForRemotePlayer())
	{
		// [서버, 원격 플레이어] 게스트의 결정을 기다리고, 신호가 끝내 오지 않으면 스텝으로 처리
		TimerManager.SetTimer(DecisionTimerHandle,
			FTimerDelegate::CreateUObject(this, &ThisClass::StartDodgeMove, false),
			DoubleTapWindow + RemoteDecisionTimeout, /*bLoop*/false);
		ListenForRemoteDecision();
	}
	else
	{
		StartDodgeMove(false);
	}
}

// 26.11.13 추가 - [로컬 조작자] 첫 입력을 뗐으면 두 번째 누름을 기다린다
void ULeeGameplayAbility_Dodge::OnFirstInputReleased(float /*TimeHeld*/)
{
	if (!IsActive() || bMoveStarted)
	{
		return;
	}

	UAbilityTask_WaitInputPress* PressTask =
		UAbilityTask_WaitInputPress::WaitInputPress(this, /*bTestAlreadyPressed*/false);
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnSecondInputPressed);
	PressTask->ReadyForActivation();
}

// 26.11.13 추가 - [로컬 조작자] 대기 시간 안에 다시 눌렀다 → 구르기
void ULeeGameplayAbility_Dodge::OnSecondInputPressed(float /*TimeWaited*/)
{
	DecideDodgeLocally(true);
}

// 26.11.13 추가 - [로컬 조작자] 대기 시간이 지났다 → 스텝
void ULeeGameplayAbility_Dodge::OnDoubleTapWindowExpired()
{
	DecideDodgeLocally(false);
}

// 26.11.13 추가 - [로컬 조작자] 최종 결정 후 게스트라면 서버에 신호를 보내고 동작을 시작한다
void ULeeGameplayAbility_Dodge::DecideDodgeLocally(bool bRoll)
{
	if (!IsActive() || bMoveStarted)
	{
		return;
	}

	// 구르기 몽타주가 없거나 스태미나가 모자라면 스텝으로 대체
	const bool bFinalRoll = bRoll && CanRoll();

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ActorInfo && ASC && !ActorInfo->IsNetAuthority())
	{
		// 게스트 → 서버: GameCustom1 = 스텝, GameCustom2 = 구르기 (콤보 전환 신호와 같은 방식)
		FScopedPredictionWindow ScopedPrediction(ASC, true);
		ASC->ServerSetReplicatedEvent(
			bFinalRoll ? EAbilityGenericReplicatedEvent::GameCustom2 : EAbilityGenericReplicatedEvent::GameCustom1,
			CurrentSpecHandle, GetCurrentActivationInfo().GetActivationPredictionKey(), ASC->ScopedPredictionKey);
	}

	StartDodgeMove(bFinalRoll);
}

// 26.11.13 추가 - [서버, 원격 플레이어] 게스트의 결정 신호를 구독하고, 이미 도착해 있으면 바로 처리
void ULeeGameplayAbility_Dodge::ListenForRemoteDecision()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	const FPredictionKey ActivationKey = GetCurrentActivationInfo().GetActivationPredictionKey();
	RemoteStepHandle = ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::GameCustom1, CurrentSpecHandle, ActivationKey)
		.AddUObject(this, &ThisClass::OnRemoteStepDecided);
	RemoteRollHandle = ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::GameCustom2, CurrentSpecHandle, ActivationKey)
		.AddUObject(this, &ThisClass::OnRemoteRollDecided);

	ASC->CallReplicatedEventDelegateIfSet(EAbilityGenericReplicatedEvent::GameCustom1, CurrentSpecHandle, ActivationKey);
	ASC->CallReplicatedEventDelegateIfSet(EAbilityGenericReplicatedEvent::GameCustom2, CurrentSpecHandle, ActivationKey);
}

// 26.11.13 추가 - [서버, 원격 플레이어] 게스트가 스텝을 골랐다
void ULeeGameplayAbility_Dodge::OnRemoteStepDecided()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ConsumeGenericReplicatedEvent(EAbilityGenericReplicatedEvent::GameCustom1, CurrentSpecHandle,
			GetCurrentActivationInfo().GetActivationPredictionKey());
	}
	StartDodgeMove(false);
}

// 26.11.13 추가 - [서버, 원격 플레이어] 게스트가 구르기를 골랐다 (스태미나는 StartDodgeMove에서 서버가 다시 확인)
void ULeeGameplayAbility_Dodge::OnRemoteRollDecided()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ConsumeGenericReplicatedEvent(EAbilityGenericReplicatedEvent::GameCustom2, CurrentSpecHandle,
			GetCurrentActivationInfo().GetActivationPredictionKey());
	}
	StartDodgeMove(true);
}

// 26.11.13 추가 - 스텝 또는 구르기 몽타주 재생 + [서버] 추가 스태미나·i-frame 타이머 (기존 ActivateAbility의 1·2단계를 옮김)
void ULeeGameplayAbility_Dodge::StartDodgeMove(bool bRoll)
{
	if (!IsActive() || bMoveStarted)
	{
		return;
	}
	bMoveStarted = true;

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	TimerManager.ClearTimer(DecisionTimerHandle);

	// 서버도 한 번 더 확인 — 스태미나가 모자라면 스텝으로 대체
	if (bRoll && !CanRoll())
	{
		bRoll = false;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const bool bIsServer = ActorInfo && ActorInfo->IsNetAuthority();

	UAnimMontage* Montage = DodgeMontage;
	FName Section = NAME_None;
	if (bRoll)
	{
		Montage = RollMontage;
		Section = RollSection;
		RotateToDodgeDirection();

		// [서버] 구르기 추가 스태미나 소모
		if (bIsServer && RollExtraCostEffect)
		{
			const FGameplayEffectSpecHandle CostSpec = MakeOutgoingGameplayEffectSpec(RollExtraCostEffect, GetAbilityLevel());
			ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, CostSpec);
		}
	}
	else
	{
		Section = SelectDodgeSection();
	}

	if (Section != NAME_None && Montage->GetSectionIndex(Section) == INDEX_NONE)
	{
		UE_LOG(LogLee, Error, TEXT("[LeeGA_Dodge] 몽타주 [%s]에 섹션 [%s]이 없습니다. BP의 섹션 이름을 확인해주세요."),
			*GetNameSafe(Montage), *Section.ToString());
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 1. 몽타주 재생
	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, Montage, /*PlayRate*/1.0f, Section);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();

	// 2. [서버] i-frame 시작/종료 타이머 — AnimNotify에 의존하지 않아 데디 서버에서도 보장
	if (bIsServer)
	{
		const float BeginTime = bRoll ? RollIFrameStartTime : IFrameStartTime;
		const float EndTime   = bRoll ? RollIFrameEndTime   : IFrameEndTime;
		TimerManager.SetTimer(IFrameBeginTimerHandle,
			FTimerDelegate::CreateUObject(this, &ThisClass::OnIFrameBegin), FMath::Max(BeginTime, KINDA_SMALL_NUMBER), /*bLoop*/false);
		TimerManager.SetTimer(IFrameEndTimerHandle,
			FTimerDelegate::CreateUObject(this, &ThisClass::OnIFrameEnd), FMath::Max(EndTime, KINDA_SMALL_NUMBER), /*bLoop*/false);
	}
}

// 26.11.13 추가 - 구르기 가능 여부 (구르기 몽타주 지정 + 추가 스태미나 GE를 적용할 수 있는지)
bool ULeeGameplayAbility_Dodge::CanRoll() const
{
	if (!RollMontage)
	{
		return false;
	}
	if (!RollExtraCostEffect)
	{
		return true;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	return ASC && ASC->CanApplyAttributeModifiers(
		RollExtraCostEffect->GetDefaultObject<UGameplayEffect>(), GetAbilityLevel(),
		MakeEffectContext(CurrentSpecHandle, CurrentActorInfo));
}

// 26.11.13 추가 - 구르기는 한 방향 몽타주라서 재생 직전에 입력 방향으로 캐릭터를 돌린다
void ULeeGameplayAbility_Dodge::RotateToDodgeDirection()
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	const FVector InputVector = GetDodgeInputVector();
	if (!Pawn || InputVector.IsNearlyZero())
	{
		return;
	}

	Pawn->SetActorRotation(FRotator(0.0f, InputVector.Rotation().Yaw, 0.0f));
}

// 26.11.13 추가 - 서버에서 다른 사람이 조작하는 캐릭터의 어빌리티인지
bool ULeeGameplayAbility_Dodge::IsServerForRemotePlayer() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	return ActorInfo && ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled();
}

// 종료 — 무적/퍼펙트 윈도우 GE와 타이머를 무조건 정리 (태그 누수 방지 안전장치) + 회복 지연 적용
void ULeeGameplayAbility_Dodge::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	CleanupDefenseWindows();

	// 26.11.13 추가 - 결정 대기 타이머와 서버 결정 신호 구독 해제 (대기 중 공격 등으로 끊겨도 남지 않게)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DecisionTimerHandle);
	}
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		const FPredictionKey ActivationKey = ActivationInfo.GetActivationPredictionKey();
		if (RemoteStepHandle.IsValid())
		{
			ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::GameCustom1, Handle, ActivationKey).Remove(RemoteStepHandle);
		}
		if (RemoteRollHandle.IsValid())
		{
			ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::GameCustom2, Handle, ActivationKey).Remove(RemoteRollHandle);
		}
	}
	RemoteStepHandle.Reset();
	RemoteRollHandle.Reset();

	// ActivationOwnedTags의 RegenBlocked는 어빌리티 종료와 동시에 사라지므로,
	// 여기서 별도 Duration GE를 걸어 "회피 종료 후 N초간" 회복 정지를 이어간다
	ApplyDurationEffect(StaminaRegenDelayEffect, StaminaRegenDelayDuration);

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
	const AActor* DbgAvatar = GetAvatarActorFromActorInfo();
	// UE_LOG(LogLee, Warning, TEXT("[임시디버그][PerfectDodge] 이벤트 수신 → Cue 실행 시도 NetMode=%d"),
	// 	DbgAvatar ? (int32)DbgAvatar->GetNetMode() : -1);

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
