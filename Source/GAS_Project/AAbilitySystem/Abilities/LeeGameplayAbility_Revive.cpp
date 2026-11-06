// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameplayAbility_Revive.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACharacter/LeeHealthComponent.h"
#include "GAS_Project/ACharacter/LeeCharacter.h"

// 상호작용으로 실행되는 로컬 예측 어빌리티 (이동 입력 취소를 클라이언트에서 바로 판단하기 위해)
ULeeGameplayAbility_Revive::ULeeGameplayAbility_Revive(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 26.11.06 추가 - 식별 태그. 가드·구르기·공격이 CancelAbilitiesWithTag로 부활을 끊는다
	AbilityTags.AddTag(MyTags::Souls::Ability_Revive);
}

// 대상 확인 → 몽타주 → ReviveDuration 대기 시작, 취소 조건 감시 시작
void ULeeGameplayAbility_Revive::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bReviveSucceeded = false;
	ReviveTarget = TriggerEventData ? const_cast<AActor*>(ToRawPtr(TriggerEventData->Target)) : nullptr;

	// 26.11.04 추가 - 부활 게이지가 남는 버그 조사용 (어느 머신에서 시작/종료되는지)
	UE_LOG(LogLee, Log, TEXT("[Revive] 시작 Reviver=%s Target=%s Authority=%d Local=%d"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(ReviveTarget.Get()), HasAuthority(&ActivationInfo), IsLocallyControlled());
	if (!IsTargetStillDowned())
	{
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
		return;
	}

	if (ReviveMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, ReviveMontage, /*Rate*/1.0f, /*StartSection*/NAME_None, /*bStopWhenAbilityEnds*/true);
		MontageTask->ReadyForActivation();
	}

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, ReviveDuration);
	DelayTask->OnFinish.AddDynamic(this, &ThisClass::HandleReviveDelayFinished);
	DelayTask->ReadyForActivation();

	GetWorld()->GetTimerManager().SetTimer(CancelCheckTimerHandle, this, &ThisClass::CheckCancelConditions, 0.05f, true);

	if (HasAuthority(&ActivationInfo))
	{
		if (ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
		{
			HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleReviverHealthChanged);
		}

		// 26.11.06 추가 - 부활하는 동안 대상의 사망 카운트다운을 멈춘다 (EndAbility에서 다시 진행)
		if (ALeeCharacter* TargetCharacter = Cast<ALeeCharacter>(ReviveTarget.Get()))
		{
			TargetCharacter->AddReviver();
			bAddedAsReviver = true;
		}
	}

	// 26.11.04 추가 - 부활시키는 사람의 화면에만 대상 가슴에 진행 게이지 표시
	if (IsLocallyControlled())
	{
		if (ALeeCharacter* TargetCharacter = Cast<ALeeCharacter>(ReviveTarget.Get()))
		{
			TargetCharacter->ShowReviveGauge(ReviveDuration);
		}
	}

	K2_OnReviveStarted(ReviveTarget.Get(), ReviveDuration);
}

// 대상 ASC에 Status.Downed가 남아 있는지 확인
bool ULeeGameplayAbility_Revive::IsTargetStillDowned() const
{
	const UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ReviveTarget.Get());
	return TargetASC && TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Downed);
}

// 이동 키 입력(로컬), 대상 사망·부활·거리 이탈 시 취소
void ULeeGameplayAbility_Revive::CheckCancelConditions()
{
	if (!IsActive())
	{
		return;
	}

	const APawn* Reviver = Cast<APawn>(GetAvatarActorFromActorInfo());
	bool bShouldCancel = !Reviver || !IsTargetStillDowned();

	// 이동 키를 누르면 부활을 멈추고 바로 움직일 수 있게 한다
	if (!bShouldCancel && Reviver->IsLocallyControlled())
	{
		bShouldCancel = !Reviver->GetLastMovementInputVector().IsNearlyZero();
	}

	if (!bShouldCancel && MaxReviveDistance > 0.0f)
	{
		bShouldCancel = FVector::Dist(Reviver->GetActorLocation(), ReviveTarget->GetActorLocation()) > MaxReviveDistance;
	}

	if (bShouldCancel)
	{
		UE_LOG(LogLee, Log, TEXT("[Revive] 취소 조건 충족 (이동/대상 상태/거리) Local=%d"), IsLocallyControlled());
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}

// 피격(HP 감소) 시 취소 (서버)
void ULeeGameplayAbility_Revive::HandleReviverHealthChanged(ULeeHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator)
{
	if (NewValue < OldValue && IsActive())
	{
		UE_LOG(LogLee, Log, TEXT("[Revive] 피격으로 취소 (서버)"));
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}

// 채널링 완료: 서버에서만 대상에게 부활 이벤트를 보낸다 (대상의 GA_Downed가 HP 복구 후 종료)
void ULeeGameplayAbility_Revive::HandleReviveDelayFinished()
{
	if (HasAuthority(&CurrentActivationInfo) && IsTargetStillDowned())
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ReviveTarget.Get()))
		{
			FGameplayEventData Payload;
			Payload.EventTag = MyTags::Souls::GameplayEvent_Revive;
			Payload.Instigator = GetAvatarActorFromActorInfo();
			Payload.Target = ReviveTarget.Get();
			TargetASC->HandleGameplayEvent(Payload.EventTag, &Payload);
		}
		bReviveSucceeded = true;
	}

	// 26.11.06 수정 - 서버만 종료를 복제한다.
	// 클라이언트(예측)가 먼저 끝나 종료를 복제하면 서버 타이머가 끝나기 전에 어빌리티가 끝나 부활 이벤트가 전송되지 않는다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, HasAuthority(&CurrentActivationInfo), false);
}

// 감시 타이머·피격 구독 해제 후 BP 종료 이벤트 호출
void ULeeGameplayAbility_Revive::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	GetWorld()->GetTimerManager().ClearTimer(CancelCheckTimerHandle);

	UE_LOG(LogLee, Log, TEXT("[Revive] 종료 Reviver=%s Target=%s Authority=%d Local=%d Cancelled=%d Succeeded=%d"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(ReviveTarget.Get()), HasAuthority(&ActivationInfo), IsLocallyControlled(), bWasCancelled, bReviveSucceeded);

	// 26.11.04 추가 - 완료/취소 모두 게이지를 숨긴다
	if (ALeeCharacter* TargetCharacter = Cast<ALeeCharacter>(ReviveTarget.Get()))
	{
		TargetCharacter->HideReviveGauge();

		// 26.11.06 추가 - [서버] 부활 취소·완료 → 사망 카운트다운 재개 (완료 시에는 넉다운이 먼저 끝나 무시됨)
		if (bAddedAsReviver)
		{
			TargetCharacter->RemoveReviver();
		}
	}
	bAddedAsReviver = false;

	if (ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &ThisClass::HandleReviverHealthChanged);
	}

	K2_OnReviveEnded(!bWasCancelled && (bReviveSucceeded || !HasAuthority(&ActivationInfo)));

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
