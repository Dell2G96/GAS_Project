// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LeeInventoryItemDefinition.h"
#include "LeeInventoryFragment_SetStats.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeInventoryFragment_SetStats : public ULeeInventoryItemFragment
{
	GENERATED_BODY()
public:
	virtual void OnInstanceCreated(class ULeeInventoryItemInstance* Instance) const override;

	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TMap<FGameplayTag, int32> InitialItemStats;
};
