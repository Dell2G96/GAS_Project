// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeHealthComponent.h"

#include "GameplayEffectExtension.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/AAbilitySystem/AttributeSets/LeeHealthSet.h"
#include "GAS_Project/AMessage/LeeVerbMessage.h"
#include "GAS_Project/AMessage/LeeVerbMessageHelpers.h"
#include "GAS_Project/System/LeeAssetManager.h"
#include "GAS_Project/System/LeeGameData.h"
#include "GameFramework/PlayerState.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"

ULeeHealthComponent::ULeeHealthComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick =false;

	SetIsReplicatedByDefault(true);

	AbilitySystemComponent = nullptr;
	HealthSet = nullptr;
	DeathState = ELeeDeathState::NotDead;
	
}

void ULeeHealthComponent::OnUnregister()
{
	UninitializeFromAbilitySystem();
	
	Super::OnUnregister();
}

void ULeeHealthComponent::InitializeWithAbilitySystem(class ULeeAbilitySystemComponent* InASC)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (AbilitySystemComponent)
	{
		UE_LOG(LogLee, Error, TEXT("LeeHealthComponent: Health component for owner [%s] has already been initialized with an ability system."), *GetNameSafe(Owner));
		return;
	}

	AbilitySystemComponent = InASC;
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogLee, Error, TEXT("LeeHealthComponent: Cannot initialize health component for owner [%s] with NULL ability system."), *GetNameSafe(Owner));
		return;
	}

	HealthSet = AbilitySystemComponent->GetSet<ULeeSoulsStatSet>();
	if (!HealthSet)
	{
		UE_LOG(LogLee, Error, TEXT("LeeHealthComponent: Cannot initialize health component for owner [%s] with NULL health set on the ability system."), *GetNameSafe(Owner));
		return;
	}

	HealthSet->OnMaxHealthChanged.AddUObject(this, &ThisClass::HandleMaxHealthChanged);
	HealthSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
	HealthSet->OnMaxStaminaChanged.AddUObject(this, &ThisClass::HandleMaxStaminaChanged);
	HealthSet->OnStaminaChanged.AddUObject(this, &ThisClass::HandleStaminaChanged);
	HealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);


	AbilitySystemComponent->SetNumericAttributeBase(ULeeSoulsStatSet::GetHealthAttribute(), HealthSet->GetMaxHealth());
	AbilitySystemComponent->SetNumericAttributeBase(ULeeSoulsStatSet::GetStaminaAttribute(), HealthSet->GetMaxStamina());

	ClearGameplayTags();

	OnHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
	OnMaxHealthChanged.Broadcast(this, HealthSet->GetMaxHealth(), HealthSet->GetMaxHealth(), nullptr);
	OnStaminaChanged.Broadcast(this, HealthSet->GetStamina(), HealthSet->GetStamina(), nullptr);
	OnMaxStaminaChanged.Broadcast(this, HealthSet->GetMaxStamina(), HealthSet->GetMaxStamina(), nullptr);
}


void ULeeHealthComponent::UninitializeFromAbilitySystem()
{
	ClearGameplayTags();

	if (HealthSet)
	{
		HealthSet->OnHealthChanged.RemoveAll(this);
		HealthSet->OnMaxHealthChanged.RemoveAll(this);
		HealthSet->OnStaminaChanged.RemoveAll(this);
		HealthSet->OnMaxStaminaChanged.RemoveAll(this);
		HealthSet->OnOutOfHealth.RemoveAll(this);
	}

	HealthSet = nullptr;
	AbilitySystemComponent = nullptr;
}

void ULeeHealthComponent::ClearGameplayTags()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(MyTags::Lyra::Status_Death_Dying, 0);
		AbilitySystemComponent->SetLooseGameplayTagCount(MyTags::Lyra::Status_Death_Dead, 0);
	}
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


float ULeeHealthComponent::GetStamina() const
{
	return (HealthSet ? HealthSet->GetStamina() : 0.0f);
}

float ULeeHealthComponent::GetMaxStamina() const
{
	return (HealthSet ? HealthSet->GetMaxStamina() : 0.0f);

}

float ULeeHealthComponent::GetStaminaNormalized() const
{
	if (HealthSet)
	{
		const float Stamina = HealthSet->GetStamina();
		const float MaxStamina = HealthSet->GetMaxStamina();

		return ((MaxStamina > 0.0f) ? (Stamina / MaxStamina) : 0.0f);
	}

	return 0.0f;
}




void ULeeHealthComponent::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser,
	const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void ULeeHealthComponent::HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser,
	const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnMaxHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void ULeeHealthComponent::HandleStaminaChanged(AActor* DamageInstigator, AActor* DamageCauser,
	const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnStaminaChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void ULeeHealthComponent::HandleMaxStaminaChanged(AActor* DamageInstigator, AActor* DamageCauser,
	const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnMaxStaminaChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void ULeeHealthComponent::HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser,
	const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	
#if WITH_SERVER_CODE
	if (AbilitySystemComponent && DamageEffectSpec)
	{
		// Send the "GameplayEvent.Death" gameplay event through the owner's ability system.  This can be used to trigger a death gameplay ability.
		{
			FGameplayEventData Payload;
			Payload.EventTag = MyTags::Souls::GameplayEvent_Death;
			Payload.Instigator = DamageInstigator;
			Payload.Target = AbilitySystemComponent->GetAvatarActor();
			Payload.OptionalObject = DamageEffectSpec->Def;
			Payload.ContextHandle = DamageEffectSpec->GetEffectContext();
			Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
			Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
			Payload.EventMagnitude = DamageMagnitude;

			FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);
			AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
		}

		// Send a standardized verb message that other systems can observe
		{
			FLeeVerbMessage Message;
			Message.Verb = MyTags::Lyra::Lyra_Elimination_Message;
			Message.Instigator = DamageInstigator;
			Message.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
			Message.Target = ULeeVerbMessageHelpers::GetPlayerStateFromObject(AbilitySystemComponent->GetAvatarActor());
			Message.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
			//@TODO: Fill out context tags, and any non-ability-system source/instigator tags
			//@TODO: Determine if it's an opposing team kill, self-own, team kill, etc...

			UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
			MessageSystem.BroadcastMessage(Message.Verb, Message);
		}

		//@TODO: assist messages (could compute from damage dealt elsewhere)?
	}

#endif // #if WITH_SERVER_CODE
}


void ULeeHealthComponent::StartDeath()
{
	if (DeathState != ELeeDeathState::NotDead)
	{
		return;
	}

	DeathState = ELeeDeathState::DeathStarted;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(MyTags::Lyra::Status_Death_Dying, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	OnDeathStarted.Broadcast(Owner);

	Owner->ForceNetUpdate();
}

void ULeeHealthComponent::FinishDeath()
{
	if (DeathState != ELeeDeathState::DeathStarted)
	{
		return;
	}

	DeathState = ELeeDeathState::DeathFinished;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(MyTags::Lyra::Status_Death_Dead, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	OnDeathFinished.Broadcast(Owner);

	Owner->ForceNetUpdate();
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


// ULeeHealthComponent* ULeeHealthComponent::FindHealthComponent(const AActor* Actor)
// {
// 	if (!Actor)
// 	{
// 		return nullptr;
// 	}
// 	ULeeHealthComponent* HealthComponent = Actor->FindComponentByClass<ULeeHealthComponent>();
// 	return HealthComponent;
// }




void ULeeHealthComponent::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	OnHealthChanged.Broadcast(this,ChangeData.OldValue, ChangeData.NewValue, GetInstigatorFromAttributeChangeData(ChangeData));
}




//
// void ULeeHealthComponent::InitializeAbilitySystem(class ULeeAbilitySystemComponent* InASC)
// {
// 	AActor* Owner = GetOwner();
// 	check(Owner);
//
// 	if (AbilitySystemComponent)
// 	{
// 		UE_LOG(LogLee, Error, TEXT("HakHealthComponent: Health component for owner [%s] has already been initialized with an ability system."), *GetNameSafe(Owner));
// 		return ;
// 	}
//
// 	AbilitySystemComponent = InASC;
// 	if (!AbilitySystemComponent)
// 	{
// 		return;
// 	}
//
// 	HealthSet = AbilitySystemComponent->GetSet<ULeeHealthSet>();
// 	if (!HealthSet)
// 	{
// 		return;
// 	}
//
// 	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(ULeeHealthSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandleHealthChanged);
//
// 	OnHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
// }