// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CGameplayAbility.h"
#include "GA_Dead.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UGA_Dead : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dead();

protected:
	UPROPERTY(EditDefaultsOnly, Category="Death")
	UAnimMontage* DeathMontage;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
