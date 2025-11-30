#include "CWeaponComponent.h"

#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "GAS_Project/Item/Weapon/CWeapon.h"
#include "Net/UnrealNetwork.h"


// UCWeaponComponent::UCWeaponComponent()
// {
// 	SetIsReplicated(true);
// }
//
// void UCWeaponComponent::RegisterSpawnedWeapon(struct FGameplayTag InWeaponTag, class ACWeapon* InWeapon, bool bRegister)
// {
// 	WeaponMap.Emplace(InWeaponTag,InWeapon);
// 	
// 	if (bRegister)
// 	{
// 		CurrentEquippedWeaponTag = InWeaponTag;
// 	}
// }
//
// ACWeapon* UCWeaponComponent::GetCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const
// {
// 	if (WeaponMap.Contains(InWeaponTagToGet))
// 	{
// 		if (ACWeapon* const* FoundWeapon = WeaponMap.Find(InWeaponTagToGet))
// 		{
// 			return *FoundWeapon;
// 		}
// 	}
//
// 	return nullptr;
// }
//
// ACWeapon* UCWeaponComponent::GetCharacterCurrentEquippedWeapon() const
// {
// 	if (!CurrentEquippedWeaponTag.IsValid())
// 	{
// 		return nullptr;
// 	}
//  
// 	return GetCarriedWeaponByTag(CurrentEquippedWeaponTag);
// }


//--------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------

UCWeaponComponent::UCWeaponComponent()
{
	SetIsReplicatedByDefault(true);
}

void UCWeaponComponent::RegisterSpawnedWeapon(FGameplayTag InWeaponTag,
											  ACWeapon* InWeapon,
											  bool bRegister)
{
	if (!InWeapon)
		return;

	// 서버에서 WeaponEntries 갱신
	if (GetOwnerRole() == ROLE_Authority)
	{
		FWeaponEntry NewEntry;
		NewEntry.WeaponTag = InWeaponTag;
		NewEntry.Weapon = InWeapon;

		WeaponEntries.Add(NewEntry);
	}

	// 서버/클라 공통으로 쓰는 로컬 맵 갱신
	WeaponMap.Emplace(InWeaponTag, InWeapon);

	if (bRegister)
	{
		CurrentEquippedWeaponTag = InWeaponTag;
	}
}

void UCWeaponComponent::OnRep_WeaponEntries()
{
	// 클라에서 배열이 갱신될 때마다 WeaponMap 재구성
	WeaponMap.Empty();

	for (const FWeaponEntry& Entry : WeaponEntries)
	{
		if (Entry.Weapon)
		{
			WeaponMap.Emplace(Entry.WeaponTag, Entry.Weapon);
		}
	}
}

ACWeapon* UCWeaponComponent::GetCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const
{
	if (ACWeapon* const* Found = WeaponMap.Find(InWeaponTagToGet))
	{
		return *Found;
	}
	return nullptr;
}

ACWeapon* UCWeaponComponent::GetCharacterCurrentEquippedWeapon() const
{
	if (!CurrentEquippedWeaponTag.IsValid())
		return nullptr;

	return GetCarriedWeaponByTag(CurrentEquippedWeaponTag);
}

void UCWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCWeaponComponent, CurrentEquippedWeaponTag);
	DOREPLIFETIME(UCWeaponComponent, WeaponEntries);
}
