// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeQuickBarComponent.h"

#include "LeeEquipmentDefinition.h"
#include "LeeEquipmentInstance.h"
#include "LeeEquipmentManagerComponent.h"
#include "GAS_Project/AInventory/LeeInventoryFragment_EquippableItem.h"
#include "GAS_Project/AInventory/LeeInventoryItemInstance.h"


ULeeQuickBarComponent::ULeeQuickBarComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void ULeeQuickBarComponent::BeginPlay()
{
	if (Slots.Num() < NumSlots)
	{
		Slots.AddDefaulted(NumSlots - Slots.Num());
	}
	
	Super::BeginPlay();
}

class ULeeEquipmentManagerComponent* ULeeQuickBarComponent::FindEquipmentManger() const
{
	if (AController* OwnerController = Cast<AController>(GetOwner()))
	{
		if (APawn* Pawn = OwnerController->GetPawn())
		{
			return Pawn->FindComponentByClass<ULeeEquipmentManagerComponent>();
		}
	}
	return nullptr;
}

void ULeeQuickBarComponent::UnequipItemInSlot()
{
	if (ULeeEquipmentManagerComponent* EquipmentManager = FindEquipmentManger())
	{
		if (EquippedItem)
		{
			EquipmentManager->UnEquipItem(EquippedItem);
			EquippedItem = nullptr;
		}
	}
}

void ULeeQuickBarComponent::EquipItemInSlot()
{
	check(Slots.IsValidIndex(ActiveSlotIndex));
	check(EquippedItem == nullptr);

	if (ULeeInventoryItemInstance* SlotItem = Slots[ActiveSlotIndex])
	{
		if (const ULeeInventoryFragment_EquippableItem* EquipInfo = SlotItem->FindFragmentByClass<ULeeInventoryFragment_EquippableItem>())
		{
			TSubclassOf<ULeeEquipmentDefinition> EquipDef = EquipInfo->EquipmentDefinition;
			if (EquipDef)
			{
				EquippedItem = FindEquipmentManger()->EquipItem(EquipDef);
				if (EquippedItem)
				{
					EquippedItem->Instigator = SlotItem;
				}
			}
		}
	}
}

void ULeeQuickBarComponent::AddItemToSlot(int32 SlotIndex, class ULeeInventoryItemInstance* Item)
{
	if (Slots.IsValidIndex(SlotIndex) && (Item != nullptr))
	{
		if (Slots[SlotIndex] == nullptr)
		{
			Slots[SlotIndex] = Item;
		}
	}
}

void ULeeQuickBarComponent::SetActiveSlotIndex(int32 NewIndex)
{
	if (Slots.IsValidIndex(NewIndex) && (ActiveSlotIndex != NewIndex))
	{
		UnequipItemInSlot();
		ActiveSlotIndex = NewIndex;
		EquipItemInSlot();
	}
}
