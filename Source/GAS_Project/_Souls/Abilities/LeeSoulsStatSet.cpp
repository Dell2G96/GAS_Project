// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeSoulsStatSet.h"

#include "GameplayEffectExtension.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AMessage/LeeVerbMessage.h"

ULeeSoulsStatSet::ULeeSoulsStatSet()
	:Health(100.f)
	, MaxHealth(100.f)
	, Stamina(100.f)
	, MaxStamina(100.f)
{
	bOutOfHealth = false;
	MaxHealthBeforeAttributeChange = 0.0f;
	HealthBeforeAttributeChange = 0.0f;
	MaxStaminaBeforeAttributeChange = 0.0f;
	StaminaBeforeAttributeChange = 0.0f;
}

bool ULeeSoulsStatSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}
	
	// Save the current health
	HealthBeforeAttributeChange = GetHealth();
	MaxHealthBeforeAttributeChange = GetMaxHealth();

	StaminaBeforeAttributeChange = GetStamina();
	MaxStaminaBeforeAttributeChange = GetMaxStamina();
	return true;}

void ULeeSoulsStatSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// Get current clamped health (from PreAttributeChange)
		float CurrentHealth = GetHealth();

		// Clamp again (defensive programming)
		float ClampedHealth = FMath::Clamp(CurrentHealth, 0.0f, 100.0f);

		// Only set if different (optimization)
		if (CurrentHealth != ClampedHealth)
		{
			SetHealth(ClampedHealth);
		}
	}
	if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		// Get current clamped health (from PreAttributeChange)
		float CurrentStamina = GetStamina();

		// Clamp again (defensive programming)
		float ClampedStamina = FMath::Clamp(CurrentStamina, 0.0f, 100.0f);

		// Only set if different (optimization)
		if (CurrentStamina != ClampedStamina)
		{
			SetStamina(ClampedStamina);
		}
	}
	const bool bIsDamageFromSelfDestruct = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(MyTags::Souls::Gameplay_DamageSelfDestruct);
    float MinimumHealth = 0.0f;

// #if !UE_BUILD_SHIPPING
//     // Godmode and unlimited health stop death unless it's a self destruct
//     if (!bIsDamageFromSelfDestruct &&
//         (Data.Target.HasMatchingGameplayTag(LeeGameplayTags::Cheat_GodMode) || Data.Target.HasMatchingGameplayTag(LeeGameplayTags::Cheat_UnlimitedHealth)))
//     {
//         MinimumHealth = 1.0f;
//     }
// #endif // #if !UE_BUILD_SHIPPING

    const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
    AActor* Instigator = EffectContext.GetOriginalInstigator();
    AActor* Causer = EffectContext.GetEffectCauser();

    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        if (Data.EvaluatedData.Magnitude > 0.0f)
        {
            FLeeVerbMessage Message;
            Message.Verb =  MyTags::Souls::Gameplay_Damage_Message;
            Message.Instigator = Data.EffectSpec.GetEffectContext().GetEffectCauser();
            Message.InstigatorTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
            Message.Target = GetOwningActor();
            Message.TargetTags = *Data.EffectSpec.CapturedTargetTags.GetAggregatedTags();
            //@TODO: Fill out context tags, and any non-ability-system source/instigator tags
            //@TODO: Determine if it's an opposing team kill, self-own, team kill, etc...
            Message.Magnitude = Data.EvaluatedData.Magnitude;

            UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
            MessageSystem.BroadcastMessage(Message.Verb, Message);
        }
        if (GetHealth() != HealthBeforeAttributeChange)
        {
            OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
        }

        if ((GetHealth() <= 0.0f) && !bOutOfHealth)
        {
            OnOutOfHealth.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
        }

        // Check health again in case an event above changed it.
        bOutOfHealth = (GetHealth() <= 0.0f);
    }
    if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
    {
        if (GetStamina() != StaminaBeforeAttributeChange)
        {
            OnStaminaChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, StaminaBeforeAttributeChange, GetStamina());
        }
    }
}
