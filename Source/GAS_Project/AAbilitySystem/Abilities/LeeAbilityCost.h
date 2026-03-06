// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "LeeGameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "UObject/NoExportTypes.h"
#include "LeeAbilityCost.generated.h"

/**
 * 
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class GAS_PROJECT_API ULeeAbilityCost : public UObject
{
	GENERATED_BODY()
public:
	ULeeAbilityCost();

	virtual bool CheckCost(const class ULeeGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
	{
		return true;
	}

	virtual void ApplyCost(const ULeeGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle , const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
	{
		
	}
	
};

