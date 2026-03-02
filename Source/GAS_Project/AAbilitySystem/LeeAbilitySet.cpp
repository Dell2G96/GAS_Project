// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeAbilitySet.h"

#include "LeeAbilitySystemComponent.h"
#include "Abilities/LeeGameplayAbility.h"

void FLeeAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FLeeAbilitySet_GrantedHandles::TakeFromAbilitySystem(class ULeeAbilitySystemComponent* LeeASC)
{
	if (!LeeASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			LeeASC->ClearAbility(Handle);
		}
	}
}

ULeeAbilitySet::ULeeAbilitySet(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	
}

void ULeeAbilitySet::GiveToAbilitySystem(ULeeAbilitySystemComponent* LeeASC,
	FLeeAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject)
{
	check(LeeASC);
	if (!LeeASC->IsOwnerActorAuthoritative())
	{
		return;	
	}

	for (int32 AbilityIndex = 0; AbilityIndex < GrantedGameplayAbilities.Num(); ++AbilityIndex)
	{
		const FLeeAbilitySet_GameplayAbility& AbilityToGrant = GrantedGameplayAbilities[AbilityIndex];
		if (!IsValid(AbilityToGrant.Ability))
		{
			continue;
		}

		ULeeGameplayAbility* AbilityCDO = AbilityToGrant.Ability->GetDefaultObject<ULeeGameplayAbility>();

		FGameplayAbilitySpec AbilitySpec(AbilityCDO, AbilityToGrant.AbilityLevel);
		AbilitySpec.SourceObject = SourceObject;
		//AbilitySpec.DynamicAbilityTags.AddTag(AbilityToGrant.InputTag);
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityToGrant.InputTag);

		const FGameplayAbilitySpecHandle AbilitySpecHandle = LeeASC->GiveAbility(AbilitySpec);
		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilitySpecHandle);
		}
	}
}
