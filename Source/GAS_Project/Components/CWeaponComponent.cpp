#include "CWeaponComponent.h"

#include "GAS_Project/Item/Weapon/CWeapon.h"


void UCWeaponComponent::RegisterSpawnedWeapon(struct FGameplayTag InWeaponTag, class ACWeapon* InWeapon, bool bRegister)
{
	WeaponMap.Emplace(InWeaponTag,InWeapon);
	
	if (bRegister)
	{
		CurrentEquippedWeaponTag = InWeaponTag;
	}
}

ACWeapon* UCWeaponComponent::GetCharacterCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const
{
	if (WeaponMap.Contains(InWeaponTagToGet))
	{
		if (ACWeapon* const* FoundWeapon = WeaponMap.Find(InWeaponTagToGet))
		{
			return *FoundWeapon;
		}
	}

	return nullptr;
}

ACWeapon* UCWeaponComponent::GetCharacterCurrentEquippedWeapon() const
{
	if (!CurrentEquippedWeaponTag.IsValid())
	{
		return nullptr;
	}
 
	return GetCharacterCarriedWeaponByTag(CurrentEquippedWeaponTag);
}
