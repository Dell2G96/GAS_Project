// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeInventoryItemDefinition.h"
#include "LeeInventoryFragment_MainIcon.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeInventoryFragment_MainIcon : public ULeeInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Apperance)
	FSlateBrush Brush;
};
