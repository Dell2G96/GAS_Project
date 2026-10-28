// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameplayAbility_Death.h"

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
