// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_GrantNearbyInteraction.generated.h"

class UGameplayAbility;
class UObject;
struct FFrame;
struct FGameplayAbilitySpecHandle;
struct FObjectKey;

UCLASS()
class GAS_PROJECT_API UAbilityTask_GrantNearbyInteraction : public UAbilityTask
{
	GENERATED_UCLASS_BODY()
	
public:
	virtual void Activate() override;
	
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta=(Hidepin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_GrantNearbyInteraction* GrantAbilitiesForNearbyInteractors(UGameplayAbility* OwningAbility, float InteractionScanRange, float InteractionScanRate);
	
private:
	virtual void OnDestroy(bool AbilityEnded) override;
	
	void QueryInteractables();

	float InteractionScanRange = 100;
	float InteractionScanRate = 0.100;

	FTimerHandle QueryTimerHandle;

	TMap<FObjectKey, FGameplayAbilitySpecHandle> InteractionAbilityCache;

};
