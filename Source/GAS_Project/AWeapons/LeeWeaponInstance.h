// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/ACosmetic/LeeCosmeticAnimationType.h"
#include "GAS_Project/AEquipment/LeeEquipmentInstance.h"
#include "UObject/Object.h"
#include "LeeWeaponInstance.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeWeaponInstance : public ULeeEquipmentInstance
{
	GENERATED_BODY()
public:
	ULeeWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintPure = false , Category= Animation)
	TSubclassOf<class UAnimInstance> PickBestAnimLayer(bool bEquipped, const FGameplayTagContainer& CosmeticTags) const;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	FLeeAnimLayerSelectionSet EquippedAnimSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	FLeeAnimLayerSelectionSet UnEquippedAnimSet;
};
