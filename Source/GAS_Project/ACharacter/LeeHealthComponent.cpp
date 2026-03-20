// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeHealthComponent.h"

#include "GameplayEffectExtension.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/AAbilitySystem/AttributeSets/LeeHealthSet.h"
#include "GAS_Project/System/LeeAssetManager.h"
#include "GAS_Project/System/LeeGameData.h"

ULeeHealthComponent::ULeeHealthComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick =false;

	AbilitySystemComponent = nullptr;
	HealthSet = nullptr;
	
}

void ULeeHealthComponent::InitializeAbilitySystem(class ULeeAbilitySystemComponent* InASC)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (AbilitySystemComponent)
	{
		UE_LOG(LogLee, Error, TEXT("HakHealthComponent: Health component for owner [%s] has already been initialized with an ability system."), *GetNameSafe(Owner));
		return ;
	}

	AbilitySystemComponent = InASC;
	if (!AbilitySystemComponent)
	{
		return;
	}

	HealthSet = AbilitySystemComponent->GetSet<ULeeHealthSet>();
	if (!HealthSet)
	{
		return;
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ULeeHealthSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);

	OnHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
}

void ULeeHealthComponent::UninitializeAbilitySystem()
{
	AbilitySystemComponent = nullptr;
	HealthSet = nullptr;
}

static AActor* GetInstigatorFromAttributeChangeData(const FOnAttributeChangeData& ChangeData)
{
	if (ChangeData.GEModData != nullptr)
	{
		const FGameplayEffectContextHandle& EffectContextHandle = ChangeData.GEModData->EffectSpec.GetEffectContext();
		return EffectContextHandle.GetOriginalInstigator();
	}
	return nullptr;
}


ULeeHealthComponent* ULeeHealthComponent::FindHealthComponent(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	ULeeHealthComponent* HealthComponent = Actor->FindComponentByClass<ULeeHealthComponent>();
	return HealthComponent;
}

float ULeeHealthComponent::GetHealth() const
{
	return (HealthSet ? HealthSet->GetHealth() : 0.f);
}

float ULeeHealthComponent::GetMaxHealth() const
{
	return ( HealthSet ? HealthSet->GetMaxHealth() : 0.f );
}

float ULeeHealthComponent::GetHealthNormalized() const
{
	if (HealthSet)
	{
		const float Health = HealthSet->GetHealth();
		const float MaxHealth = HealthSet->GetMaxHealth();
		return ((MaxHealth > 0.f) ? (Health / MaxHealth) : 0.f);
	}
	return 0.f;
}

void ULeeHealthComponent::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	OnHealthChanged.Broadcast(this,ChangeData.OldValue, ChangeData.NewValue, GetInstigatorFromAttributeChangeData(ChangeData));
}

void ULeeHealthComponent::DamageSelfDestruct(bool bFellOutOfWorld)
{
	if ((DeathState == ELeeDeathState::NotDead) && AbilitySystemComponent)
	{
		const TSubclassOf<UGameplayEffect> DamageGE = ULeeAssetManager::GetSubclass(ULeeGameData::Get().DamageGameplayEffect_SetByCaller);

		if (!DamageGE)
		{
			UE_LOG(LogLee, Error, TEXT("LyraHealthComponent: DamageSelfDestruct failed for owner [%s]. Unable to find gameplay effect [%s]."), *GetNameSafe(GetOwner()), *ULeeGameData::Get().DamageGameplayEffect_SetByCaller.GetAssetName());
			return;
		}

		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DamageGE, 1.f, AbilitySystemComponent->MakeEffectContext());
		FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

		if (!Spec)
		{
			UE_LOG(LogLee, Error, TEXT("LyraHealthComponent: DamageSelfDestruct failed for owner [%s]. Unable to make outgoing spec for [%s]."), *GetNameSafe(GetOwner()), *GetNameSafe(DamageGE));
			return;
		}

		Spec->AddDynamicAssetTag(MyTags::Souls::Gameplay_DamageSelfDestruct);

		if (bFellOutOfWorld)
		{
			Spec->AddDynamicAssetTag(MyTags::Souls::Gameplay_FellOutOfWorld);
		}
		const float DamageAmount = -GetMaxHealth();

		Spec->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Damage, DamageAmount);
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec);
	}
}

