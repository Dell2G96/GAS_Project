// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "LeeInventoryItemDefinition.h"
#include "LeeInventoryFragment_PickupIcon.generated.h"


class UObject;
class USkeletalMesh;


UCLASS()
class GAS_PROJECT_API ULeeInventoryFragment_PickupIcon : public ULeeInventoryItemFragment
{
	GENERATED_BODY()

public:
	ULeeInventoryFragment_PickupIcon();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Apperance)
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Apperance)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Apperance)
	FLinearColor PadColor;
	
};
