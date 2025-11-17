#include "CWeaponComponent.h"

#include "GAS_Project/Item/Weapon/CWeapon.h"

void UCWeaponComponent::RegisterSpawnedWeapon(struct FGameplayTag InWeaponTag, class ACWeapon* InWeapon, bool bRegister)
{
	WeaponMap.Emplace(InWeaponTag, InWeapon);

	if (bRegister)
	{
		CurrentWeaponTag = InWeaponTag;
	}
	
}

class ACWeapon* UCWeaponComponent::GetWeaponByTag(struct FGameplayTag InWeaponTag) const
{
	if (WeaponMap.Contains(InWeaponTag))
	{
		if (ACWeapon* const* FoundWeapon = WeaponMap.Find(InWeaponTag))
		{
			return *FoundWeapon;
		}
	}
	return nullptr;
}

class ACWeapon* UCWeaponComponent::GetCurrentWeapon() const
{
	if (!CurrentWeaponTag.IsValid())
	{
		return nullptr;
	}
	return GetWeaponByTag(CurrentWeaponTag);
}

