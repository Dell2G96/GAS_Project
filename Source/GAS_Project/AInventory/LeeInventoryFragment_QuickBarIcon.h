// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "LeeInventoryItemDefinition.h"
#include "Styling/SlateBrush.h"

#include "LeeInventoryFragment_QuickBarIcon.generated.h"

class UObject;


UCLASS()
class GAS_PROJECT_API ULeeInventoryFragment_QuickBarIcon : public ULeeInventoryItemFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apperance")
	FSlateBrush Brush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apperance")
	FSlateBrush AmmoBrush;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apperance")
	FText DisplayNameWhenEquipped;
};
