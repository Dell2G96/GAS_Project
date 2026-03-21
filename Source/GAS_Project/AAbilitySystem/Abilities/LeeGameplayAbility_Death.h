// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_Death.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_Death : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_Death(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category="Lee|Ability")
	void StartDeath();
	
	UFUNCTION(BlueprintCallable, Category="Lee|Ability")
	void FinishDeath();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|Death")
	bool bAutoStartDeath;
};
