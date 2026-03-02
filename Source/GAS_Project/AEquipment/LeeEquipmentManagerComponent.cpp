// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeEquipmentManagerComponent.h"

#include "LeeEquipmentDefinition.h"
#include "LeeEquipmentInstance.h"

ULeeEquipmentInstance* FLeeEquipmentList::AddEntry(TSubclassOf<ULeeEquipmentDefinition> EquipmentDefinition)
{
	ULeeEquipmentInstance* Result = nullptr;
	check(EquipmentDefinition != nullptr);
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());

	const ULeeEquipmentDefinition* EquipmentCDO = GetDefault<ULeeEquipmentDefinition>(EquipmentDefinition);

	TSubclassOf<ULeeEquipmentInstance> InstanceType = EquipmentCDO->InstanceType;
	if (!InstanceType)
	{
		InstanceType = ULeeEquipmentInstance::StaticClass();
	}

	FLeeAppliedEquipmentEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.EquipmentDefinition = EquipmentDefinition;
	NewEntry.Instance = NewObject<ULeeEquipmentInstance>(OwnerComponent->GetOwner(), InstanceType);
	Result = NewEntry.Instance;

	Result->SpawnEquipmentActors(EquipmentCDO->ActorToSpawns);

	return Result;
}

void FLeeEquipmentList::RemoveEntry(ULeeEquipmentInstance* Instance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FLeeAppliedEquipmentEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			Instance->DestroyEquipmentActors();
			EntryIt.RemoveCurrent();
		}
	}
}

ULeeEquipmentManagerComponent::ULeeEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
	, EquipmentList(this)
{
}

class ULeeEquipmentInstance* ULeeEquipmentManagerComponent::EquipItem(
	TSubclassOf<class ULeeEquipmentDefinition> EquipmentDefinition)
{
	ULeeEquipmentInstance* Result = nullptr;
	if (EquipmentDefinition)
	{
		Result = EquipmentList.AddEntry(EquipmentDefinition);
		if (Result)
		{
			Result->OnEquipped();
		}
	}
	return Result;
}

void ULeeEquipmentManagerComponent::UnEquipItem(ULeeEquipmentInstance* ItemInstance)
{
	if (ItemInstance)
	{
		ItemInstance->OnUnEquipped();
		EquipmentList.RemoveEntry(ItemInstance);
	}
}
