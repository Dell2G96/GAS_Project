// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CGameplayAbility.h"
#include "GA_AttackMelee.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UGA_AttackMelee : public UCGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_AttackMelee();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;


	UFUNCTION()
	void OnGameplayEventTaskReceived(FGameplayEventData Payload);

	FGameplayTag GetAttackEventTag();

	UPROPERTY(EditDefaultsOnly, Category="GAS|EventTag")
	FGameplayTag HitActorEventTag;
	

	UPROPERTY(EditDefaultsOnly, Category="GAS|Montage")
	class UAnimMontage* AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category="GAS|Damage")
	class TSubclassOf<UGameplayEffect> DamageEffect;

	
};
