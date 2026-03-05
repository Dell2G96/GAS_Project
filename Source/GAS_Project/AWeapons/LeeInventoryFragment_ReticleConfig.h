// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/AInventory/LeeInventoryFragment_EquippableItem.h"
#include "LeeInventoryFragment_ReticleConfig.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeInventoryFragment_ReticleConfig : public ULeeInventoryFragment_EquippableItem
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Reticle)
	TArray<TSubclassOf<class ULeeReticleWidgetBase>> ReticleWidgets;
};
