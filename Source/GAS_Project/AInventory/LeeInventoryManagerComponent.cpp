// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeInventoryManagerComponent.h"

#include "LeeInventoryItemDefinition.h"
#include "LeeInventoryItemInstance.h"
#include "Net/UnrealNetwork.h"


//////////////////////////////////////////////////////////////////////
// FLyraInventoryEntry
FString FLeeInventoryEntry::GetDebugString() const
{
	TSubclassOf<ULeeInventoryItemDefinition> ItemDef;
	if (Instance != nullptr)
	{
		ItemDef = Instance->GetItemDef();
	}
	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *GetNameSafe(ItemDef));
	
}


//////////////////////////////////////////////////////////////////////
// FLyraInventoryList

void FLeeInventoryList::AddEntry(ULeeInventoryItemInstance* Instance)
{
	unimplemented();
}

void FLeeInventoryList::RemoveEntry(ULeeInventoryItemInstance* Instance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FLeeInventoryEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			EntryIt.RemoveCurrent();

			// NetWorkCOde
			//MarkArrayDirty();
		}
	}
	
}


TArray<ULeeInventoryItemInstance*> FLeeInventoryList::GetAllItem() const
{
	TArray<ULeeInventoryItemInstance*> Results;
	Results.Reserve(Entries.Num());
	for (const FLeeInventoryEntry& Entry : Entries)
	{
		if (Entry.Instance != nullptr) //@TODO: Would prefer to not deal with this here and hide it further?
		{
			Results.Add(Entry.Instance);
		}
	}
	return Results;
}



ULeeInventoryItemInstance* FLeeInventoryList::AddEntry(TSubclassOf<class ULeeInventoryItemDefinition> ItemDef)
{
	ULeeInventoryItemInstance* Result = nullptr;
	check(ItemDef);
	check(OwnerComponent);

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	FLeeInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Instance = NewObject<ULeeInventoryItemInstance>(OwningActor);
	NewEntry.Instance->ItemDef = ItemDef;

	for (const ULeeInventoryItemFragment* Fragment : GetDefault<ULeeInventoryItemDefinition>(ItemDef)->Fragments)
	{
		if (Fragment)
		{
			Fragment->OnInstanceCreated(NewEntry.Instance);
		}
	}

	Result = NewEntry.Instance;
	return Result;
	
}

//////////////////////////////////////////////////////////////////////
// ULeeInventoryManagerComponent

ULeeInventoryManagerComponent::ULeeInventoryManagerComponent(const FObjectInitializer& InObjectInitializer)
	:Super(InObjectInitializer)
	,InventoryList(this) 
{
}

void ULeeInventoryManagerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}



ULeeInventoryItemInstance* ULeeInventoryManagerComponent::AddItemDefinition(
	TSubclassOf<class ULeeInventoryItemDefinition> ItemDef)
{
	ULeeInventoryItemInstance* Result = nullptr;
	if (ItemDef)
	{
		Result = InventoryList.AddEntry(ItemDef);
	}
	return Result;
}

void ULeeInventoryManagerComponent::AddItemInstance(ULeeInventoryItemInstance* ItemInstance)
{
	InventoryList.AddEntry(ItemInstance);
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && ItemInstance)
	{
		AddReplicatedSubObject(ItemInstance);
	}
}

void ULeeInventoryManagerComponent::RemoveItemInstance(ULeeInventoryItemInstance* ItemInstance)
{
	InventoryList.RemoveEntry(ItemInstance);

	if (ItemInstance && IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(ItemInstance);
	}
}

TArray<ULeeInventoryItemInstance*> ULeeInventoryManagerComponent::GetAllItems() const
{
	return InventoryList.GetAllItem();

}
