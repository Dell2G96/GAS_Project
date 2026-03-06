// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeAbilityCost.h"
#include "LeeAbilityCost_ItemTagStack.generated.h"

/**
 * 
 */
UCLASS(meta=(DisplayName = "Item Tag Stack"))
class GAS_PROJECT_API ULeeAbilityCost_ItemTagStack : public ULeeAbilityCost
{
	GENERATED_BODY()
public:
	ULeeAbilityCost_ItemTagStack();

	virtual bool CheckCost(const class ULeeGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ApplyCost(const ULeeGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= Costs)
	FScalableFloat Quantity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= Costs)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= Costs)
	FGameplayTag FailureTag;
};
