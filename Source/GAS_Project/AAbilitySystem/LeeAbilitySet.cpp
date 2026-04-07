// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeAbilitySet.h"

#include "LeeAbilitySystemComponent.h"
#include "Abilities/LeeGameplayAbility.h"
#include "GAS_Project/LeeLogChannels.h"

void FLeeAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FLeeAbilitySet_GrantedHandles::AddGameplayeEffectHandle(const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		GameplayEffectHandles.Add(Handle);
	}
}

void FLeeAbilitySet_GrantedHandles::AddAttributeSet(UAttributeSet* Set)
{
	GrantedAttributeSets.Add(Set);
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
	FLeeAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject) const
{
	check(LeeASC);
	if (!LeeASC->IsOwnerActorAuthoritative())
	{
		return;	
	}
	
	//Grant The Attibute Sets
	for (int32 SetIndex = 0; SetIndex < GrantedAttributes.Num(); ++SetIndex)
	{
		const FLeeAbilitySet_AttributeSet& SetToGrant = GrantedAttributes[SetIndex];
		if (!IsValid(SetToGrant.AttributeSet))
		{
			UE_LOG(LogLee, Error, TEXT("GrantedAttributes[%d] on ability set [%s] is not valid"), SetIndex, *GetNameSafe(this));
			continue;
		}
		
		UAttributeSet* NewSet = NewObject<UAttributeSet>(LeeASC->GetOwner(), SetToGrant.AttributeSet);
		LeeASC->AddAttributeSetSubobject(NewSet);
		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAttributeSet(NewSet);
		}
	}
	
	// 게임어빌리티 부여 
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
	
	for (int32 EffectIndex = 0; EffectIndex < GrantedGameplayEffects.Num(); ++EffectIndex)
	{
		const FLeeAbilitySet_GameplayEffect& EffectToGrant = GrantedGameplayEffects[EffectIndex];

		if (!IsValid(EffectToGrant.GameplayEffect))
		{
			UE_LOG(LogLee, Error, TEXT("GrantedGameplayEffects[%d] on ability set [%s] is not valid"), EffectIndex, *GetNameSafe(this));
			continue;
		}

		const UGameplayEffect* GameplayEffect = EffectToGrant.GameplayEffect->GetDefaultObject<UGameplayEffect>();
		const FActiveGameplayEffectHandle GameplayEffectHandle = LeeASC->ApplyGameplayEffectToSelf(GameplayEffect, EffectToGrant.EffectLevel, LeeASC->MakeEffectContext());

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddGameplayeEffectHandle(GameplayEffectHandle);
		}
	}
}
