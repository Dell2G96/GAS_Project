// Fill out your copyright notice in the Description page of Project Settings.


#include "CPlayerWeaponComponent.h"

#include "GAS_Project/Item/Weapon/CPlayerWeapon.h"

ACPlayerWeapon* UCPlayerWeaponComponent::GetHeroCarriedWeaponByTag(FGameplayTag InWeaponTag) const
{
	return Cast<ACPlayerWeapon>(GetCharacterCarriedWeaponByTag((InWeaponTag)));
}

ACPlayerWeapon* UCPlayerWeaponComponent::GetCurrentWeapon() const
{
	return Cast<ACPlayerWeapon>(GetCharacterCurrentEquippedWeapon());
}

float UCPlayerWeaponComponent::GetHeroCurrentEquippWeaponDamageAtLevel(float InLevel) const
{
	return 0.f;
}
