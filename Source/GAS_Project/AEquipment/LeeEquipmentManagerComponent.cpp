// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeEquipmentManagerComponent.h"

#include "AbilitySystemGlobals.h"
#include "LeeEquipmentDefinition.h"
#include "LeeEquipmentInstance.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"

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

	ULeeAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	check(ASC);
	{
		for (const TObjectPtr<ULeeAbilitySet> AbilitySet : EquipmentCDO->AbilitySetsToGrant)
		{
			AbilitySet->GiveToAbilitySystem(ASC, &NewEntry.GrantedHandles, Result);
		}
	}

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
			ULeeAbilitySystemComponent* ASC = GetAbilitySystemComponent();
			check(ASC);
			{
				Entry.GrantedHandles.TakeFromAbilitySystem(ASC);
			}
			
			Instance->DestroyEquipmentActors();
			EntryIt.RemoveCurrent();
		}
	}
}

ULeeAbilitySystemComponent* FLeeEquipmentList::GetAbilitySystemComponent() const
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();

	return Cast<ULeeAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor));
	
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

ULeeEquipmentInstance* ULeeEquipmentManagerComponent::GetFirstInstanceOfType(
	TSubclassOf<ULeeEquipmentInstance> InstanceType)
{
	for (FLeeAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (ULeeEquipmentInstance* Instance = Entry.Instance)
		{
			if (Instance->IsA(InstanceType))
			{
				return Instance;
			}
		}
	}
	return nullptr;
}

TArray<class ULeeEquipmentInstance*> ULeeEquipmentManagerComponent::GetEquipmentInstancesOfType(
	TSubclassOf<class ULeeEquipmentInstance> InstanceType) const
{
	TArray<ULeeEquipmentInstance*> Result;

	for (const FLeeAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (ULeeEquipmentInstance* Instance = Entry.Instance)
		{
			if (Instance->IsA(InstanceType))
			{
				Result.Add(Instance);
				
			}
		}
		
	}
	return Result;
}
