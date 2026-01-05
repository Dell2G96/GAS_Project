// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "CAbilitySystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API UCAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UCAbilitySystemComponent();
	void InitializeBaseAttributes();
	void ServerSideInit();
	void ApplyFullStatEffect();
	
	const TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>>& GetAbilities() const;

	UFUNCTION(BlueprintCallable, Category="GAS|Abilities")
	bool TryActivateAbilityByTag(FGameplayTag AbilityTagToActivate);
	
private:
	void ApplyInitialEffects();
	void GiveInitialAbilities();
	void AuthApplyGameplayEffect(TSubclassOf<UGameplayEffect> GameplayEffect, int Level = 1);
	void HealthUpdate(const FOnAttributeChangeData& ChangeData);
	void StaminaUpdate(const FOnAttributeChangeData& ChangeData);

	FTimerHandle TimeLimitToRevive;
	FActiveGameplayEffectHandle KnockdownGEHandle;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Gameplay Abilitys")
	TSubclassOf<class UGameplayEffect> KnockdownEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Gameplay Abilitys")
	TSubclassOf<class UGameplayEffect> DeadEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Gameplay Abilitys")
	TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>> Abilities;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Gameplay Abilitys")
	TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>> BasicAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Gameplay Ability")
	class UDA_AbilitySystemGenerics* AbilitySystemGenerics;

	// 추가
public:
	void ApplyKnockdown();
	void RemoveKnockdown();

	bool TryRevive(float ReviveHealthRatio = 0.3f);
	void ApplyDeath();
	void RemoveDeath();

	bool IsPlayer();
};



