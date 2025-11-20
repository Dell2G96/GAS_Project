// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DA_AbilitySystemGenerics.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UDA_AbilitySystemGenerics : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	FORCEINLINE TSubclassOf<class UGameplayEffect> GetFullStatEffect() const { return FullStatEffect; }
	FORCEINLINE TSubclassOf<class UGameplayEffect> GetDeathEffect() const { return DeathEffect; }
	FORCEINLINE const TArray<TSubclassOf<class UGameplayEffect>>& GetInitialEffects() const { return InitialEffects; }
	FORCEINLINE const TArray<TSubclassOf<class UGameplayAbility>>& GetPassiveAbilities() const { return PassiveAbilities; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Gameplay Effects")
	TSubclassOf<class UGameplayEffect> FullStatEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Gameplay Effects")
	TSubclassOf<class UGameplayEffect> DeathEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Gameplay Effects")
	TArray<TSubclassOf<class UGameplayEffect>> InitialEffects;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Gameplay Abilitys")
	TArray<TSubclassOf<class UGameplayAbility>> PassiveAbilities;
	
};
