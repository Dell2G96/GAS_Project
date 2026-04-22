// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_BeAssassinated.generated.h"

UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_BeAssassinated : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_BeAssassinated(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Assassination")
	UAnimMontage* VictimMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Assassination")
	TSubclassOf<class UGameplayEffect> InvincibleGE;

	UPROPERTY(EditDefaultsOnly, Category = "Assassination")
	FGameplayTag AssassinationStartEventTag;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	FActiveGameplayEffectHandle ActiveInvincibleGEHandle;
	bool bAddedAssassinatedTag = false;
};
