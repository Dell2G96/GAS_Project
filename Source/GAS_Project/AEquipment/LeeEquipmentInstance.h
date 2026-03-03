// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeEquipmentDefinition.h"
#include "GAS_Project/AInventory/LeeInventoryItemDefinition.h"
#include "UObject/Object.h"
#include "LeeEquipmentInstance.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class GAS_PROJECT_API ULeeEquipmentInstance : public ULeeInventoryItemFragment
{
	GENERATED_BODY()
	
public:
	ULeeEquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintImplementableEvent, Category= Equipment, meta=(DisplayName = "OnEquipped"))
	void K2_OnEquipped();

	UFUNCTION(BlueprintImplementableEvent, Category= Equipment, meta=(DisplayName = "OnUnEquipped"))
	void K2_OnUnequipped();

	UFUNCTION(BlueprintPure, Category = Equipment)
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }

	UFUNCTION(BlueprintPure, Category=Equipment)
	APawn* GetPawn() const;

	void SpawnEquipmentActors(const TArray<FLeeEquipmentActorToSpawn>& ActorsToSpawn);
	void DestroyEquipmentActors();

	UFUNCTION(BlueprintPure, Category=Equipment, meta=(DeterminesOutputType = PawnType))
	APawn* GetTypedPawn(TSubclassOf<APawn> PawnType) const;

	virtual void OnEquipped();
	virtual void OnUnEquipped();

	UPROPERTY()
	TObjectPtr<UObject> Instigator;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;
	
	// UPROPERTY(EditAnywhere, Category= Lee)
	// TSubclassOf<class ULeeEquipmentDefinition> EquipmentDefinition;

};
