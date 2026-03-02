// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LeeInventoryManagerComponent.generated.h"

USTRUCT(BlueprintType)
struct FLeeInventoryEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<class ULeeInventoryItemInstance> Instance = nullptr;
	
};


//					FLeeInventoryList
USTRUCT(BlueprintType)
struct FLeeInventoryList
{
	GENERATED_BODY()

	FLeeInventoryList(UActorComponent* InOwnerComponent = nullptr) : OwnerComponent
		(InOwnerComponent)
	{
	}

	ULeeInventoryItemInstance* AddEntry(TSubclassOf<class ULeeInventoryItemDefinition> ItemDef);

	UPROPERTY()
	TArray<FLeeInventoryEntry> Entries;

	UPROPERTY()
	TObjectPtr<UActorComponent> OwnerComponent;
	
};




UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	ULeeInventoryManagerComponent(const FObjectInitializer& InObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Inventory")
	ULeeInventoryItemInstance* AddItemDefinition(TSubclassOf<class ULeeInventoryItemDefinition> ItemDef);

	UPROPERTY()
	FLeeInventoryList InventoryList;
};
