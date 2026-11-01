// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeInventoryItemInstance.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
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


// 26.11.01 수정 - 인벤토리 항목 1개. FastArray 항목으로 바꿔 클라이언트로 복제된다
USTRUCT(BlueprintType)
struct FLeeInventoryEntry : public FFastArraySerializerItem
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
// 26.11.01 수정 - 인벤토리 목록. FastArray로 소유 클라이언트에 복제된다
USTRUCT(BlueprintType)
struct FLeeInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FLeeInventoryList(UActorComponent* InOwnerComponent = nullptr) : OwnerComponent
		(InOwnerComponent)
	{
	}
	TArray<ULeeInventoryItemInstance*> GetAllItem() const ;

	ULeeInventoryItemInstance* AddEntry(TSubclassOf<class ULeeInventoryItemDefinition> ItemDef, int32 StackCount);
	void AddEntry(ULeeInventoryItemInstance* Instance);

	void RemoveEntry(ULeeInventoryItemInstance* Instance);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FLeeInventoryEntry, FLeeInventoryList>(Entries, DeltaParms, *this);
	}

public:
	friend ULeeInventoryManagerComponent;
	
	UPROPERTY()
	TArray<FLeeInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
	
};

template<>
struct TStructOpsTypeTraits<FLeeInventoryList> : public TStructOpsTypeTraitsBase2<FLeeInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	ULeeInventoryManagerComponent(const FObjectInitializer& InObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Inventory")
	ULeeInventoryItemInstance* AddItemDefinition(TSubclassOf<class ULeeInventoryItemDefinition> ItemDef,int32 StackCount = 1);

	// UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	// bool CanAddItemDefinition(TSubclassOf<ULeeInventoryItemDefinition> ItemDef, int32 StackCount = 1);
	
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddItemInstance(ULeeInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void RemoveItemInstance(ULeeInventoryItemInstance* ItemInstance);





	

	UFUNCTION(BlueprintCallable,BlueprintPure=false ,Category=Inventory)
	TArray<ULeeInventoryItemInstance*> GetAllItems() const;

	//~UActorComponent interface
	virtual void ReadyForReplication() override;
	//~End of UActorComponent interface

	UPROPERTY(Replicated)
	FLeeInventoryList InventoryList;
};
