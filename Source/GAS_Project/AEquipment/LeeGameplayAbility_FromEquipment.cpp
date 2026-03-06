 // Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameplayAbility_FromEquipment.h"

#include "GAS_Project/AInventory/LeeInventoryItemInstance.h"

 ULeeEquipmentInstance* ULeeGameplayAbility_FromEquipment::GetAssociatedEquipment() const
{
	if (FGameplayAbilitySpec* Spec = UGameplayAbility::GetCurrentAbilitySpec())
	{
		return Cast<ULeeEquipmentInstance>(Spec->SourceObject.Get());
	}
	return nullptr;
}

 ULeeInventoryItemInstance* ULeeGameplayAbility_FromEquipment::GetAssociatedItem() const
 {
	if (ULeeEquipmentInstance* Equipment = GetAssociatedEquipment())
	{
		
		return Cast<ULeeInventoryItemInstance>(Equipment->GetInstigator());
	}
	return nullptr;
 }
