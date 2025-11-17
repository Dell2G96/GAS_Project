// Fill out your copyright notice in the Description page of Project Settings.


#include "CAbilitySystemComponent.h"
#include "GAS_Project/Utils/CStructTypes.h"



void UCAbilitySystemComponent::GrantHeroWeaponAbilities(const TArray<struct FPlayerAbilitySet>& InAbilitySets,
	TArray<struct FGameplayAbilitySpecHandle>& OutGrantedAbilitySpecHandles, int32 ApplyLevel)
{
	
	if (InAbilitySets.IsEmpty())
	{
		return;
	}

	for (const FPlayerAbilitySet& AbilitySet : InAbilitySets)
	{
		if(!AbilitySet.IsValid()) continue;

		FGameplayAbilitySpec AbilitySpec(AbilitySet.AbilityToGrant);
		AbilitySpec.SourceObject = GetAvatarActor();
		AbilitySpec.Level = ApplyLevel;
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilitySet.InputTag);

		OutGrantedAbilitySpecHandles.AddUnique(GiveAbility(AbilitySpec));
	}

	// for (const FWarriorHeroSpecialAbilitySet& AbilitySet : InSpecialWeaponAbilities)
	// {
	// 	if(!AbilitySet.IsValid()) continue;
	//
	// 	FGameplayAbilitySpec AbilitySpec(AbilitySet.AbilityToGrant);
	// 	AbilitySpec.SourceObject = GetAvatarActor();
	// 	AbilitySpec.Level = ApplyLevel;
	// 	AbilitySpec.DynamicAbilityTags.AddTag(AbilitySet.InputTag);
	//
	// 	OutGrantedAbilitySpecHandles.AddUnique(GiveAbility(AbilitySpec));
	// }
}