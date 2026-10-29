// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameplayAbility_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/ACharacter/LeeHealthComponent.h"

ULeeGameplayAbility_Death::ULeeGameplayAbility_Death(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	bAutoStartDeath = true;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = MyTags::Souls::GameplayEvent_Death;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
		
	}
}

void ULeeGameplayAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);

	ULeeAbilitySystemComponent* LeeASC = CastChecked<ULeeAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());

	FGameplayTagContainer AbilityTypesToIgnore;
	AbilityTypesToIgnore.AddTag(MyTags::Souls::Ability_Behavior_SurvivesDeath);

	LeeASC->CancelAbilities(nullptr, &AbilityTypesToIgnore, this);

	SetCanBeCanceled(false);

	// Super를 먼저 호출하여 ActivationGroup 카운트를 증가시킨 후 ChangeActivationGroup 호출
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ChangeActivationGroup(ELeeAbilityActivationGroup::Exclusive_Blocking))
	{
		UE_LOG(LogLee, Error, TEXT("ULeeGameplayAbility_Death::ActivateAbility: Ability [%s] failed to change activation group to blocking."), *GetName());
	}

	// 26.10.29 추가 - 사망 완료 시 이 어빌리티가 종료되도록 구독 (SetCanBeCanceled(false)라 Cancel로는 끝나지 않음)
	if (ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		HealthComponent->OnDeathFinished.AddUniqueDynamic(this, &ThisClass::HandleDeathFinished);
	}

	if (bAutoStartDeath)
	{
		StartDeath();
	}

	PlayDeathMontage(TriggerEventData);
}

// 사망 몽타주 재생 — 어빌리티가 먼저 끝나도(GA_Hero_Death는 바로 EndAbility) 몽타주는 계속 재생되도록 bStopWhenAbilityEnds=false
void ULeeGameplayAbility_Death::PlayDeathMontage(const FGameplayEventData* TriggerEventData)
{
	// 이벤트 데이터의 OptionalObject2는 const 포인터 — 몽타주 에셋을 읽기만 하므로 const를 떼서 태스크에 넘긴다
	UAnimMontage* DeathMontage = TriggerEventData
		? const_cast<UAnimMontage*>(Cast<UAnimMontage>(TriggerEventData->OptionalObject2))
		: nullptr;
	if (!DeathMontage)
	{
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DeathMontage, /*Rate*/1.0f, /*StartSection*/NAME_None, /*bStopWhenAbilityEnds*/false);
	MontageTask->ReadyForActivation();
}

// 사망 완료 이벤트 수신 → 구독 해제 후 어빌리티 종료
void ULeeGameplayAbility_Death::HandleDeathFinished(AActor* OwningActor)
{
	if (ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(OwningActor))
	{
		HealthComponent->OnDeathFinished.RemoveDynamic(this, &ThisClass::HandleDeathFinished);
	}

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void ULeeGameplayAbility_Death::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{

	check(ActorInfo);
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULeeGameplayAbility_Death::StartDeath()
{
	if (ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		if (HealthComponent->GetDeathState() == ELeeDeathState::NotDead)
		{
			HealthComponent->StartDeath();
		}
	}
}

void ULeeGameplayAbility_Death::FinishDeath()
{
	if (ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		if (HealthComponent->GetDeathState() == ELeeDeathState::DeathStarted)
		{
			HealthComponent->FinishDeath();
		}
	}
}
