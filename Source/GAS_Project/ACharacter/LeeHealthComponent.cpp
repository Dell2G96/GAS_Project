// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeHealthComponent.h"

#include "GameplayEffectExtension.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/AAbilitySystem/AttributeSets/LeeHealthSet.h"

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

