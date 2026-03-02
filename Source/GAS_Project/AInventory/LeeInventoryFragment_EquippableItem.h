// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeInventoryItemDefinition.h"
#include "UObject/Object.h"
#include "LeeInventoryFragment_EquippableItem.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeInventoryFragment_EquippableItem : public ULeeInventoryItemFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere,Category=Lee)
	TSubclassOf<class ULeeEquipmentDefinition> EquipmentDefinition;
};
