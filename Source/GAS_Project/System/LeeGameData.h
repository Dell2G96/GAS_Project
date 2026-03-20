// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LeeGameData.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "Lee Game Data", ShortTooltip = "Data asset containing global game data."))
class GAS_PROJECT_API ULeeGameData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULeeGameData();

	static const ULeeGameData& Get();
	
	UPROPERTY(EditDefaultsOnly, Category="Default Gameplay Effects" , meta=(DisplayName = "Damage Gameplay Effect (SetByCaller"))
	TSoftClassPtr<class UGameplayEffect> DamageGameplayEffect_SetByCaller;

	UPROPERTY(EditDefaultsOnly, Category="Default Gameplay Effects" , meta=(DisplayName = "Heal Gameplay Effect (SetByCaller"))
	TSoftClassPtr<class UGameplayEffect> HealGameplayEffect_SetByCaller;

	UPROPERTY(EditDefaultsOnly, Category="Default Gameplay Effects")
	TSoftClassPtr<class UGameplayEffect> DynamicTagGameplayEffect;


};
