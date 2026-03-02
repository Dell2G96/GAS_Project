// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeInventoryManagerComponent.h"

#include "LeeInventoryItemDefinition.h"
#include "LeeInventoryItemInstance.h"

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

	Result = NewEntry.Instance;
	return Result;
	
}

ULeeInventoryManagerComponent::ULeeInventoryManagerComponent(const FObjectInitializer& InObjectInitializer)
	:Super(InObjectInitializer)
	,InventoryList(this) 
{
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
