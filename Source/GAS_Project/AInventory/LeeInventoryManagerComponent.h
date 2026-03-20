// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeInventoryItemInstance.h"
#include "Components/ActorComponent.h"
#include "LeeInventoryManagerComponent.generated.h"


USTRUCT(BlueprintType)
struct FLeeInventoryChangeMessage
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<UActorComponent> InventoryOwner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TObjectPtr<ULeeInventoryItemInstance> Instance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 NewCount = 0;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 Delta = 0;
};


USTRUCT(BlueprintType)
struct FLeeInventoryEntry
{
	GENERATED_BODY()

	FLeeInventoryEntry()
	{}

	FString GetDebugString() const ;

private:
	friend struct FLeeInventoryList;
	friend class ULeeInventoryManagerComponent;
	
	UPROPERTY()
	TObjectPtr<class ULeeInventoryItemInstance> Instance = nullptr;

	UPROPERTY()
	int32 StackCount = 0;
	
	
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
	TArray<ULeeInventoryItemInstance*> GetAllItem() const ;

	ULeeInventoryItemInstance* AddEntry(TSubclassOf<class ULeeInventoryItemDefinition> ItemDef);
	void AddEntry(ULeeInventoryItemInstance* Instance);

	void RemoveEntry(ULeeInventoryItemInstance* Instance);

public:
	friend ULeeInventoryManagerComponent;
	
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

	// UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	// bool CanAddItemDefinition(TSubclassOf<ULeeInventoryItemDefinition> ItemDef, int32 StackCount = 1);
	
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddItemInstance(ULeeInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void RemoveItemInstance(ULeeInventoryItemInstance* ItemInstance);





	

	UFUNCTION(BlueprintCallable,BlueprintPure=false ,Category=Inventory)
	TArray<ULeeInventoryItemInstance*> GetAllItems() const;

	UPROPERTY(Replicated)
	FLeeInventoryList InventoryList;
};
