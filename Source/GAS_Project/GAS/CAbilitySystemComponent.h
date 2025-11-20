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
	
	const TMap<ECabilityInputID, TSubclassOf<UGameplayAbility>>& GetAbilities() const;
	
private:
	void ApplyInitialEffects();
	void GiveInitialAbilities();
	void AuthApplyGameplayEffect(TSubclassOf<UGameplayEffect> GameplayEffect, int Level = 1);
	void HealthUpdate(const FOnAttributeChangeData& ChangeData);
	void ManaUpdate(const FOnAttributeChangeData& ChangeData);

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Gameplay Abilitys")
	TMap<ECabilityInputID, TSubclassOf<UGameplayAbility>> Abilities;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Gameplay Abilitys")
	TMap<ECabilityInputID, TSubclassOf<UGameplayAbility>> BasicAbilities;
	
};
