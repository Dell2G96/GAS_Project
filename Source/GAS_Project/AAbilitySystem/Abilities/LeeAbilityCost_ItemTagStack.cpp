// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeAbilityCost_ItemTagStack.h"

#include "NativeGameplayTags.h"
#include "GAS_Project/AEquipment/LeeGameplayAbility_FromEquipment.h"
#include "GAS_Project/AInventory/LeeInventoryItemInstance.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_ABILITY_FAIL_COST, "Ability.ActivateFail.Cost")

ULeeAbilityCost_ItemTagStack::ULeeAbilityCost_ItemTagStack()
	:Super()
{
	Quantity.SetValue(1.0);
	FailureTag = TAG_ABILITY_FAIL_COST;
}

bool ULeeAbilityCost_ItemTagStack::CheckCost(const class ULeeGameplayAbility* Ability,
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (const ULeeGameplayAbility_FromEquipment* EquipmentAbility = Cast<const ULeeGameplayAbility_FromEquipment>(Ability))
	{
		if (ULeeInventoryItemInstance* ItemInstance = EquipmentAbility->GetAssociatedItem())
		{
			const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

			const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
			const int32 NumStacks = FMath::TruncToInt(NumStacksReal);
			const bool bCanApplyCost = ItemInstance->GetStatTagStackCount(Tag) >= NumStacks;

			if (!bCanApplyCost && OptionalRelevantTags && FailureTag.IsValid())
			{
				OptionalRelevantTags->AddTag(FailureTag);	
			}
			return bCanApplyCost;
		}
	}
	return false;
}

void ULeeAbilityCost_ItemTagStack::ApplyCost(const ULeeGameplayAbility* Ability,
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (const ULeeGameplayAbility_FromEquipment* EquipmentAbility = Cast<const ULeeGameplayAbility_FromEquipment>(Ability))
	{
		if (ULeeInventoryItemInstance* ItemInstance = EquipmentAbility->GetAssociatedItem())
		{
			const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);
			const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
			const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

			ItemInstance->RemoveStatTagstack(Tag, NumStacks);
		}
	}
}
