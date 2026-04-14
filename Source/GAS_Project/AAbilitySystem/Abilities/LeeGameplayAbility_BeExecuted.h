// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_BeExecuted.generated.h"

UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_BeExecuted : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_BeExecuted(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Execution")
	UAnimMontage* VictimMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Execution")
	TSubclassOf<class UGameplayEffect> InvincibleGE;

	UFUNCTION()
	void OnMontageCompleted();

	FActiveGameplayEffectHandle ActiveInvincibleGEHandle;
};
