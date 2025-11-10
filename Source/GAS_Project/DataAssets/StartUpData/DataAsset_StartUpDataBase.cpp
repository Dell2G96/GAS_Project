// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAsset_StartUpDataBase.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/Abilities/CGameplayAbility.h"
#include "GameplayAbilitySpec.h"


void UDataAsset_StartUpDataBase::GiveToAbilitySystemComponent(class UCAbilitySystemComponent* InASCToGive,
                                                              int32 ApplyLevel)
{
	check(InASCToGive);

	GrantAbilities(ActivateOnGivenAbilities, InASCToGive, ApplyLevel);
	GrantAbilities(ReactiveAbilities, InASCToGive, ApplyLevel);
}

void UDataAsset_StartUpDataBase::GrantAbilities(const TArray<TSubclassOf<UCGameplayAbility>>& InAbilitiesToGrant,
	UCAbilitySystemComponent* InASCToGive, int32 ApplyLevel)
{
	if (InAbilitiesToGrant.IsEmpty())
	{
		return;
	}

	for (const TSubclassOf<UCGameplayAbility>& Ability : InAbilitiesToGrant)
	{
		if (!Ability) return;

		FGameplayAbilitySpec AbilitySpec(Ability);
		AbilitySpec.SourceObject = InASCToGive->GetAvatarActor();
		AbilitySpec.Level = ApplyLevel;

		InASCToGive->GiveAbility(AbilitySpec);
		
	}
}
