// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeWeaponUserInterface.h"

#include "GAS_Project/AEquipment/LeeEquipmentManagerComponent.h"
#include "GAS_Project/AWeapons/LeeWeaponInstance.h"

ULeeWeaponUserInterface::ULeeWeaponUserInterface(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void ULeeWeaponUserInterface::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		if (ULeeEquipmentManagerComponent* EquipmentManager = Pawn->FindComponentByClass<ULeeEquipmentManagerComponent>() )
		{
			if (ULeeWeaponInstance* NewInstance = EquipmentManager->GetFirstInstanceOfType<ULeeWeaponInstance>())
			{
				if (NewInstance != CurrentInstance && NewInstance->GetInstigator() != nullptr
					)
				{
					ULeeWeaponInstance* OldWeapon = CurrentInstance;
					CurrentInstance = NewInstance;
					OnWeaponChanged(OldWeapon, CurrentInstance);
				}
			}
		}	
	}
}
