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

	if (!ChangeActivationGroup(ELeeAbilityActivationGroup::Exclusive_Blocking))
	{
		UE_LOG(LogLee, Error, TEXT("ULyraGameplayAbility_Death::ActivateAbility: Ability [%s] failed to change activation group to blocking."), *GetName());
	}

	if (bAutoStartDeath)
	{
		StartDeath();
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
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
