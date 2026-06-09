// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeWeaponInstance.h"

ULeeWeaponInstance::ULeeWeaponInstance(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

TSubclassOf<class UAnimInstance> ULeeWeaponInstance::PickBestAnimLayer(bool bEquipped,
	const FGameplayTagContainer& CosmeticTags) const
{
	const FLeeAnimLayerSelectionSet& SetToQuery = (bEquipped ? EquippedAnimSet : UnEquippedAnimSet);
	return SetToQuery.SelectBestLayer(CosmeticTags);
}
