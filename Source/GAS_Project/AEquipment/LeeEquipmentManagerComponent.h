// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "UObject/Object.h"
#include "LeeEquipmentManagerComponent.generated.h"

USTRUCT(BlueprintType)
struct FLeeAppliedEquipmentEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<class ULeeEquipmentDefinition> EquipmentDefinition;

	UPROPERTY()
	TObjectPtr<class ULeeEquipmentInstance> Instance = nullptr;
	
};

USTRUCT(BlueprintType)
struct FLeeEquipmentList
{
	GENERATED_BODY()

	FLeeEquipmentList(class UActorComponent* InOwnerComponent = nullptr)
		:OwnerComponent(InOwnerComponent)
	{
		
	}

	class ULeeEquipmentInstance* AddEntry(TSubclassOf<ULeeEquipmentDefinition> EquipmentDefinition);
	void RemoveEntry(ULeeEquipmentInstance* Instance); 

	UPROPERTY()
	TArray<FLeeAppliedEquipmentEntry> Entries;
	
	UPROPERTY()
	TObjectPtr<UActorComponent> OwnerComponent;
	
};


UCLASS()
class GAS_PROJECT_API ULeeEquipmentManagerComponent : public UPawnComponent
{
	GENERATED_BODY()
	
public:
	ULeeEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	class ULeeEquipmentInstance* EquipItem(TSubclassOf<class ULeeEquipmentDefinition> EquipmentDefinition);
	void UnEquipItem(ULeeEquipmentInstance* ItemInstance);

	UPROPERTY()
	FLeeEquipmentList EquipmentList;
};
