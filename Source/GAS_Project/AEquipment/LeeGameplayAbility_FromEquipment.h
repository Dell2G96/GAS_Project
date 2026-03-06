// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeEquipmentInstance.h"
#include "GAS_Project/AAbilitySystem/Abilities/LeeGameplayAbility.h"
#include "LeeGameplayAbility_FromEquipment.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_FromEquipment : public ULeeGameplayAbility
{
	GENERATED_BODY()
public:
	ULeeEquipmentInstance* GetAssociatedEquipment() const;
	ULeeInventoryItemInstance* GetAssociatedItem() const;
};
