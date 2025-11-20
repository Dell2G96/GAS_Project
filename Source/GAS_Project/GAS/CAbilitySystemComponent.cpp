// Fill out your copyright notice in the Description page of Project Settings.


#include "CAbilitySystemComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "CAbilitySystemStatics.h"
#include "CAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GAS_Project/Utils/CStructTypes.h"


UCAbilitySystemComponent::UCAbilitySystemComponent()
{
	GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetHealthAttribute()).AddUObject(this, &UCAbilitySystemComponent::HealthUpdate);
	GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetManaAttribute()).AddUObject(this, &UCAbilitySystemComponent::ManaUpdate);
	
}

void UCAbilitySystemComponent::InitializeBaseAttributes()
{
	return;
	
}

void UCAbilitySystemComponent::ServerSideInit()
{
	InitializeBaseAttributes();
	ApplyInitialEffects();
	GiveInitialAbilities();
}

void UCAbilitySystemComponent::ApplyFullStatEffect()
{
	return;
}

const TMap<ECabilityInputID, TSubclassOf<UGameplayAbility>>& UCAbilitySystemComponent::GetAbilities() const
{
	return Abilities;
}


void UCAbilitySystemComponent::ApplyInitialEffects()
{
	// 오너가 없거나 서버가 아니라면 리턴 종료
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;
	
}

// CAbilitySystemComponent.cpp - GiveInitialAbilities 수정
void UCAbilitySystemComponent::GiveInitialAbilities()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	UE_LOG(LogTemp, Error, TEXT("=== GiveInitialAbilities START ==="));

	for (const TPair<ECabilityInputID, TSubclassOf<UGameplayAbility>>& AbilityPair : Abilities)
	{
		FGameplayAbilitySpecHandle Handle = GiveAbility(FGameplayAbilitySpec(AbilityPair.Value, 0, (int32)AbilityPair.Key, nullptr));
		UE_LOG(LogTemp, Error, TEXT("  [Abilities] InputID=%d | Ability=%s"), 
			(int32)AbilityPair.Key, 
			*AbilityPair.Value->GetName());
	}

	for (const TPair<ECabilityInputID, TSubclassOf<UGameplayAbility>>& AbilityPair : BasicAbilities)
	{
		FGameplayAbilitySpecHandle Handle = GiveAbility(FGameplayAbilitySpec(AbilityPair.Value, 1, (int32)AbilityPair.Key, nullptr));
		UE_LOG(LogTemp, Error, TEXT("  [BasicAbilities] InputID=%d | Ability=%s"), 
			(int32)AbilityPair.Key, 
			*AbilityPair.Value->GetName());
	}

	UE_LOG(LogTemp, Error, TEXT("=== GiveInitialAbilities END ==="));
}


void UCAbilitySystemComponent::AuthApplyGameplayEffect(TSubclassOf<UGameplayEffect> GameplayEffect, int Level)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingSpec(GameplayEffect, Level, MakeEffectContext());
		ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
		
	}
}

void UCAbilitySystemComponent::HealthUpdate(const FOnAttributeChangeData& ChangeData)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	bool bFound = false;

	float Maxhealth = GetGameplayAttributeValue(UCAttributeSet::GetMaxHealthAttribute(),bFound);

	// 피가 다 차있으면
	if (bFound && ChangeData.NewValue >= Maxhealth)
	{
		if (!HasMatchingGameplayTag(UCAbilitySystemStatics::GetHealthFullStatTag()))
		{
			AddLooseGameplayTag(UCAbilitySystemStatics::GetHealthFullStatTag());
		}	
	}
	else
	{
		RemoveLooseGameplayTag(UCAbilitySystemStatics::GetHealthFullStatTag());
	}


	// 죽으면
	if (ChangeData.NewValue <= 0)
	{
		if (!HasMatchingGameplayTag(UCAbilitySystemStatics::GetHealthEmptyStatTag()))
		{
			AddLooseGameplayTag(UCAbilitySystemStatics::GetHealthEmptyStatTag());
			
			
			FGameplayEventData DeadAbilityEventData;
			if (ChangeData.GEModData)
				DeadAbilityEventData.ContextHandle = ChangeData.GEModData->EffectSpec.GetContext();
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), UCAbilitySystemStatics::GetDeadStatTag(), DeadAbilityEventData); 
		}
	}
	else
	{
		RemoveLooseGameplayTag(UCAbilitySystemStatics::GetHealthEmptyStatTag());
	}
}

void UCAbilitySystemComponent::ManaUpdate(const FOnAttributeChangeData& ChangeData)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	bool bFound = false;

	float MaxMana = GetGameplayAttributeValue(UCAttributeSet::GetMaxManaAttribute(),bFound);


	// 피가 다 차있으면
	if (bFound && ChangeData.NewValue >= MaxMana)
	{
		if (!HasMatchingGameplayTag(UCAbilitySystemStatics::GetManaFullStatTag()))
		{
			AddLooseGameplayTag(UCAbilitySystemStatics::GetManaFullStatTag());
		}	
	}
	else
	{
		RemoveLooseGameplayTag(UCAbilitySystemStatics::GetManaFullStatTag());
	}


	// 죽으면
	if (ChangeData.NewValue <= 0)
	{
		if (!HasMatchingGameplayTag(UCAbilitySystemStatics::GetManaEmptyStatTag()))
		{
			AddLooseGameplayTag(UCAbilitySystemStatics::GetManaEmptyStatTag());
		}
	}
	else
	{
		RemoveLooseGameplayTag(UCAbilitySystemStatics::GetManaEmptyStatTag());
	}
}

