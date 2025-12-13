// Fill out your copyright notice in the Description page of Project Settings.


#include "CAbilitySystemComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "CAbilitySystemStatics.h"
#include "CAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/DataAssets/DA_AbilitySystemGenerics.h"
#include "GAS_Project/Utils/CStructTypes.h"


UCAbilitySystemComponent::UCAbilitySystemComponent()
{
	GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetHealthAttribute()).AddUObject(this, &UCAbilitySystemComponent::HealthUpdate);
	GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetStaminaAttribute()).AddUObject(this, &UCAbilitySystemComponent::StaminaUpdate);	
	
}

void UCAbilitySystemComponent::InitializeBaseAttributes()
{
	if (!AbilitySystemGenerics || !GetOwner())
	{
		return;
	}

	const FHeroBaseStats* BaseStats = nullptr;
	
	if (BaseStats)
	{
		SetNumericAttributeBase(UCAttributeSet::GetMaxHealthAttribute(), BaseStats->BaseMaxHealth);
		SetNumericAttributeBase(UCAttributeSet::GetMaxStaminaAttribute(), BaseStats->BaseMaxStamina);
	}
}

void UCAbilitySystemComponent::ServerSideInit()
{
	InitializeBaseAttributes();
	ApplyInitialEffects();
	GiveInitialAbilities();
}

void UCAbilitySystemComponent::ApplyInitialEffects()
{
	// 오너가 없거나 서버가 아니라면 리턴 종료
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	if (!AbilitySystemGenerics)
		return;

	for (const TSubclassOf<UGameplayEffect>& EffectClass : AbilitySystemGenerics->GetInitialEffects())
	{
		FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingSpec(EffectClass, 1, MakeEffectContext());
		ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	}
	
}

void UCAbilitySystemComponent::ApplyFullStatEffect()
{
	if (!AbilitySystemGenerics) return ;
	AuthApplyGameplayEffect(AbilitySystemGenerics->GetFullStatEffect());
}

const TMap<ECabilityInputID, TSubclassOf<UGameplayAbility>>& UCAbilitySystemComponent::GetAbilities() const
{
	return Abilities;
}



void UCAbilitySystemComponent::GiveInitialAbilities()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

    UE_LOG(LogTemp, Log, TEXT("=== GiveInitialAbilities START on %s ==="), *GetOwner()->GetName());
	

    // 일반 능력
	for (const TPair<ECabilityInputID,TSubclassOf<UGameplayAbility>>& AbilityPair : Abilities)
	{
		if (!*AbilityPair.Value)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ASC] Abilities[%d] is null class."), (int32)AbilityPair.Key);
			continue;
		}
		
		GiveAbility(FGameplayAbilitySpec(AbilityPair.Value, 0, (int32)AbilityPair.Key, nullptr));
	}

    // 기본 능력
	for (const TPair<ECabilityInputID,TSubclassOf<UGameplayAbility>>& AbilityPair : BasicAbilities)
	{
		if (!*AbilityPair.Value)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ASC] BasicAbilities[%d] is null class."), (int32)AbilityPair.Key);
			continue;
		}
		GiveAbility(FGameplayAbilitySpec(AbilityPair.Value, 1, (int32)AbilityPair.Key, nullptr));
	}
    // 5) 패시브 능력
	if (!AbilitySystemGenerics)
	{
		UE_LOG(LogTemp, Error, TEXT("[ASC] AbilitySystemGenerics is NULL on %s. "
			  "BP에서 UCAbilitySystemComponent의 AbilitySystemGenerics를 꼭 지정하세요."),
			  *GetOwner()->GetName());
		return;
	}
	for (const TSubclassOf<UGameplayAbility>& PassiveAbility : AbilitySystemGenerics->GetPassiveAbilities())
	{
		GiveAbility(FGameplayAbilitySpec(PassiveAbility, 1, -1, nullptr));
	}

    UE_LOG(LogTemp, Log, TEXT("=== GiveInitialAbilities END on %s ==="), *GetOwner()->GetName());
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

			if(AbilitySystemGenerics && AbilitySystemGenerics->GetDeathEffect())
				AuthApplyGameplayEffect(AbilitySystemGenerics->GetDeathEffect());

			FGameplayEventData DeadAbilityEventData;
			if(ChangeData.GEModData)
				DeadAbilityEventData.ContextHandle = ChangeData.GEModData->EffectSpec.GetContext();

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), UCAbilitySystemStatics::GetDeadStatTag(), DeadAbilityEventData);
		}
	}
	else
	{
		RemoveLooseGameplayTag(UCAbilitySystemStatics::GetHealthEmptyStatTag());
	}
}

void UCAbilitySystemComponent::StaminaUpdate(const FOnAttributeChangeData& ChangeData)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	bool bFound = false;

	float MaxStamina = GetGameplayAttributeValue(UCAttributeSet::GetMaxStaminaAttribute(),bFound);


	// 피가 다 차있으면
	if (bFound && ChangeData.NewValue >= MaxStamina)
	{
		if (!HasMatchingGameplayTag(UCAbilitySystemStatics::GetStaminaFullStatTag()))
		{
			AddLooseGameplayTag(UCAbilitySystemStatics::GetStaminaFullStatTag());
		}	
	}
	else
	{
		RemoveLooseGameplayTag(UCAbilitySystemStatics::GetStaminaFullStatTag());
	}
	
	// 스태미나 고갈 시
	if (ChangeData.NewValue <= 0)
	{
		if (!HasMatchingGameplayTag(UCAbilitySystemStatics::GetStaminaEmptyStatTag()))
		{
			AddLooseGameplayTag(UCAbilitySystemStatics::GetStaminaEmptyStatTag());
		}
	}
	else
	{
		RemoveLooseGameplayTag(UCAbilitySystemStatics::GetStaminaEmptyStatTag());
	}
}

