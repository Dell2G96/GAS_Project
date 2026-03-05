// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeReticleWidgetBase.h"

#include "GAS_Project/AInventory/LeeInventoryItemInstance.h"
#include "GAS_Project/AWeapons/LeeWeaponInstance.h"

ULeeReticleWidgetBase::ULeeReticleWidgetBase(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	
}

void ULeeReticleWidgetBase::InitializeFromWeapon(class ULeeWeaponInstance* InWeapon)
{
	WeaponInstance = InWeapon;
	InventoryInstance = nullptr;
	if (WeaponInstance)
	{
		InventoryInstance = Cast<ULeeInventoryItemInstance>(WeaponInstance->GetInstigator());
	}
	OnWeaponInitialized();
}
