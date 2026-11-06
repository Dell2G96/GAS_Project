// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameplayAbility_Downed.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/ACharacter/LeeCharacter.h"
#include "GAS_Project/ACharacter/LeeHealthComponent.h"

// 넉다운 이벤트로 트리거되는 서버 시작 어빌리티 (서버 + 소유 클라이언트에서 실행)
ULeeGameplayAbility_Downed::ULeeGameplayAbility_Downed(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = MyTags::Souls::GameplayEvent_Downed;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

// 진행 중인 행동을 끊고 넉다운 상태 진입: 태그 부여 → 임시 HP → 제한 시간 / 부활 대기 → 넉다운 몽타주
void ULeeGameplayAbility_Downed::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ULeeAbilitySystemComponent* LeeASC = CastChecked<ULeeAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());

	// 공격·가드·포션 등 진행 중인 어빌리티를 모두 취소 (사망 후에도 유지되는 어빌리티는 제외)
	FGameplayTagContainer AbilityTypesToIgnore;
	AbilityTypesToIgnore.AddTag(MyTags::Souls::Ability_Behavior_SurvivesDeath);
	LeeASC->CancelAbilities(nullptr, &AbilityTypesToIgnore, this);

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo());

	if (HasAuthority(&ActivationInfo))
	{
		// Status.Downed: 서버 태그 + 복제 태그 (클라이언트 이동속도·ABP·입력 차단에서 사용)
		LeeASC->AddLooseGameplayTag(MyTags::Souls::Status_Downed);
		LeeASC->AddReplicatedLooseGameplayTag(MyTags::Souls::Status_Downed);

		if (HealthComponent)
		{
			HealthComponent->SetHealthDirect(HealthComponent->GetMaxHealth() * DownedHealthRatio);
			HealthComponent->OnDeathStarted.AddUniqueDynamic(this, &ThisClass::HandleDeathStarted);
		}

		// 26.11.06 수정 - 사망 카운트다운을 캐릭터가 관리 (부활 채널링 중 일시정지 + 남은 시간을 클라이언트에 복제)
		if (ALeeCharacter* DownedCharacter = GetLeeCharacterFromActorInfo())
		{
			DownedCharacter->StartDownedCountdown(DownedDuration);
			CountdownExpiredHandle = DownedCharacter->OnDownedCountdownExpired.AddUObject(this, &ThisClass::HandleDownedTimeout);
		}

		// 동료의 GA_Revive가 보내는 부활 이벤트 대기
		UAbilityTask_WaitGameplayEvent* WaitReviveTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, MyTags::Souls::GameplayEvent_Revive, nullptr, /*OnlyTriggerOnce*/true);
		WaitReviveTask->EventReceived.AddDynamic(this, &ThisClass::HandleRevived);
		WaitReviveTask->ReadyForActivation();
	}

	// 무릎 꿇는 몽타주 (캐릭터에 지정된 경우만, 어빌리티가 끝나면 함께 정지)
	if (const ALeeCharacter* LeeCharacter = GetLeeCharacterFromActorInfo())
	{
		if (UAnimMontage* DownedMontage = LeeCharacter->DownedMontage)
		{
			UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, DownedMontage, /*Rate*/1.0f, /*StartSection*/NAME_None, /*bStopWhenAbilityEnds*/true);
			MontageTask->ReadyForActivation();
		}
	}
}

// 부활 성공: HP를 부활 비율로 설정하고 넉다운 종료 (서버)
void ULeeGameplayAbility_Downed::HandleRevived(FGameplayEventData Payload)
{
	if (ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		HealthComponent->SetHealthDirect(HealthComponent->GetMaxHealth() * ReviveHealthRatio);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

// 제한 시간 안에 부활하지 못함 → 사망 이벤트 (Death 어빌리티가 이 어빌리티를 취소한다)
void ULeeGameplayAbility_Downed::HandleDownedTimeout()
{
	if (ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		HealthComponent->TriggerDeathWithoutDamage();
	}
}

// 사망이 시작되면 넉다운 상태를 정리한다
void ULeeGameplayAbility_Downed::HandleDeathStarted(AActor* OwningActor)
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

// 종료 시 타이머·구독·Status.Downed 태그 정리 (서버)
void ULeeGameplayAbility_Downed::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (HasAuthority(&ActivationInfo))
	{
		// 26.11.06 수정 - 캐릭터의 사망 카운트다운 정지 + 구독 해제
		if (ALeeCharacter* DownedCharacter = GetLeeCharacterFromActorInfo())
		{
			DownedCharacter->OnDownedCountdownExpired.Remove(CountdownExpiredHandle);
			DownedCharacter->StopDownedCountdown();
		}
		CountdownExpiredHandle.Reset();

		if (ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
		{
			HealthComponent->OnDeathStarted.RemoveDynamic(this, &ThisClass::HandleDeathStarted);
		}

		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveLooseGameplayTag(MyTags::Souls::Status_Downed);
			ASC->RemoveReplicatedLooseGameplayTag(MyTags::Souls::Status_Downed);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
