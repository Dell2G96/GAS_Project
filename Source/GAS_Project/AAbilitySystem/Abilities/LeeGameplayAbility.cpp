// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemLog.h"
#include "LeeAbilityCost.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/ACharacter/LeeCharacter.h"
#include "GAS_Project/ACharacter/LeeHeroComponent.h"
#include "GAS_Project/APlayer/LeePlayerController.h"

#define ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(FunctionName, ReturnValue)																				\
{																																						\
if (!ensure(IsInstantiated()))																														\
{																																					\
ABILITY_LOG(Error, TEXT("%s: " #FunctionName " cannot be called on a non-instanced ability. Check the instancing policy."), *GetPathName());	\
return ReturnValue;																																\
}																																					\
}


ULeeGameplayAbility::ULeeGameplayAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	ActivationPolicy = ELeeAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

}

void ULeeGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	K2_OnAbilityAdded();

	TryActivateAbilityOnSpawn(ActorInfo, Spec);
}

class ULeeAbilitySystemComponent* ULeeGameplayAbility::GetLeeAbilitySystemComponentFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<ULeeAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr);
}

class ALeePlayerController* ULeeGameplayAbility::GetLeePlayerControllerFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<ALeePlayerController>(CurrentActorInfo->PlayerController.Get()) : nullptr);
}

class ALeeCharacter* ULeeGameplayAbility::GetLeeCharacterFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<ALeeCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

class ULeeHeroComponent* ULeeGameplayAbility::GetHeroComponentFromActorInfo() const
{
	return (CurrentActorInfo ? ULeeHeroComponent::FindHeroComponent(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

void ULeeGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo,
                                                    const FGameplayAbilitySpec& Spec) const
{
	if (ActorInfo && !Spec.IsActive() && (ActivationPolicy == ELeeAbilityActivationPolicy::OnSpawn))
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		const AActor* AvatarActor = ActorInfo->AvatarActor.Get();

		if (ASC && AvatarActor && !AvatarActor->GetTearOff() && (AvatarActor->GetLifeSpan() <= 0.f))
		{
			const bool bIsLocalExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted) || (EGameplayAbilityNetExecutionPolicy::LocalOnly);
			const bool bIsServerExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerInitiated);

			const bool bClientShouldActivate = ActorInfo->IsLocallyControlled() && bIsLocalExecution;

			const bool bServerShouldActivate = ActorInfo->IsNetAuthority() && bIsServerExecution;

			if (bClientShouldActivate || bServerShouldActivate)
			{
				ASC->TryActivateAbility(Spec.Handle);
			}
		}
	}
}

bool ULeeGameplayAbility::CanChangeActivationGroup(ELeeAbilityActivationGroup NewGroup) const
{
	if (!IsInstantiated() || !IsActive())
	{
		return false;
	}

	if (ActivationGroup == NewGroup)
	{
		return true;
	}

	ULeeAbilitySystemComponent* LyraASC = GetLeeAbilitySystemComponentFromActorInfo();
	check(LyraASC);

	if ((ActivationGroup != ELeeAbilityActivationGroup::Exclusive_Blocking) && LyraASC->IsActivationGroupBlocked(NewGroup))
	{
		// This ability can't change groups if it's blocked (unless it is the one doing the blocking).
		return false;
	}

	if ((NewGroup == ELeeAbilityActivationGroup::Exclusive_Replaceable) && !CanBeCanceled())
	{
		// This ability can't become replaceable if it can't be canceled.
		return false;
	}

	return true;
}

bool ULeeGameplayAbility::ChangeActivationGroup(ELeeAbilityActivationGroup NewGroup)
{
	ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(ChangeActivationGroup, false);

	if (!CanChangeActivationGroup(NewGroup))
	{
		return false;
	}

	if (ActivationGroup != NewGroup)
	{
		ULeeAbilitySystemComponent* LyraASC = GetLeeAbilitySystemComponentFromActorInfo();
		check(LyraASC);

		LyraASC->RemoveAbilityFromActivationGroup(ActivationGroup, this);
		LyraASC->AddAbilityToActivationGroup(NewGroup, this);

		ActivationGroup = NewGroup;
	}

	return true;
}

bool ULeeGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                    FGameplayTagContainer* OptionalRelevantTags) const
{
	 if(!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	 {
		return false;		 
	 }
	for (TObjectPtr<ULeeAbilityCost> AdditionalCost : AdditionalCosts)
	{
		if (AdditionalCost != nullptr)
		{
			if (!AdditionalCost->CheckCost(this, Handle, ActorInfo, OptionalRelevantTags))
			{
				return false;
			}
		}
	}
	return true;
	 	
}

void ULeeGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
	check(ActorInfo);

	for (TObjectPtr<ULeeAbilityCost> AdditionalCost : AdditionalCosts)
	{
		if (AdditionalCost != nullptr)
		{
			AdditionalCost->ApplyCost(this, Handle, ActorInfo, ActivationInfo);;
		}
	}
}
