#include "CWeaponComponent.h"

#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "GAS_Project/Item/Weapon/CWeapon.h"


void UCWeaponComponent::EquipWeapon(TSubclassOf<class ACWeapon> NewWeapon)
{
	OwnerCharacter = Cast<ACPlayerCharacter>(GetOwner());
	if (!OwnerCharacter) return;

	
	// WeaponToEquip = (GetWorld()->SpawnActor<ACWeapon>(NewWeapon));
	// WeaponToEquip->AttachToComponent( OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);


	//OwnerCharacter->GetMesh()->SetAnimClass(WeaponToEquip->CurrentWeaponConfig.AnimClass);
	//OwnerCharacter->GetMesh()->SetAnimationMode(EAnimationMode::Type::AnimationBlueprint);
	//OwnerCharacter->GetMesh()->SetAnimInstanceClass(WeaponToEquip->CurrentWeaponConfig.AnimClass);

	FActorSpawnParameters Params;
	Params.Owner = OwnerCharacter;
	Params.Instigator = Cast<APawn>(OwnerCharacter);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	 = GetWorld()->SpawnActor<ACWeapon>(WeaponToEquip, FTransform::Identity, Params);
	if (!NewWeapon) return;
	
	
}

void UCWeaponComponent::UnEquipWeapon(TSubclassOf<class ACWeapon> NewWeapon)
{
	if(!WeaponToEquip) return;
	// OwnerCharacter->GetMesh()

	
}

void UCWeaponComponent::RegisterSpawnedWeapon(struct FGameplayTag InWeaponTag, class ACWeapon* InWeapon, bool bRegister)
{
	WeaponMap.Emplace(InWeaponTag,InWeapon);
	
	if (bRegister)
	{
		CurrentEquippedWeaponTag = InWeaponTag;
	}
}

ACWeapon* UCWeaponComponent::GetCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const
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
 
	return GetCarriedWeaponByTag(CurrentEquippedWeaponTag);
}
