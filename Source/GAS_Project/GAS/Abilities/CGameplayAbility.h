// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CGameplayAbility.generated.h"


UENUM(BlueprintType)
enum class AbilityActivationPolicy : uint8
{
	OnTriggered,
	OnGiven
};

UCLASS()
class GAS_PROJECT_API UCGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category="A_|Ability")
	AbilityActivationPolicy ActivationPolicy = AbilityActivationPolicy::OnTriggered;
	

	
};
