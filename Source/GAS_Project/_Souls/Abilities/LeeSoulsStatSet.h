// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS_Project/AAbilitySystem/AttributeSets/LeeAttributeSet.h"
#include "LeeSoulsStatSet.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeSoulsStatSet : public ULeeAttributeSet
{
	GENERATED_BODY()

public:
	ULeeSoulsStatSet();

	ATTRIBUTE_ACCESSORS(ULeeSoulsStatSet, Health);
	ATTRIBUTE_ACCESSORS(ULeeSoulsStatSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(ULeeSoulsStatSet, Stamina);
	ATTRIBUTE_ACCESSORS(ULeeSoulsStatSet, MaxStamina);



	UPROPERTY(BlueprintReadOnly, Category = "Lyra|RPGStats")
	FGameplayAttributeData Health;


	UPROPERTY(BlueprintReadOnly, Category = "Lyra|RPGStats")
	FGameplayAttributeData MaxHealth;


	UPROPERTY(BlueprintReadOnly, Category = "Lyra|RPGStats")
	FGameplayAttributeData Stamina;


	UPROPERTY(BlueprintReadOnly, Category = "Lyra|RPGStats")
	FGameplayAttributeData MaxStamina;

	mutable FLeeAttributeEvent OnHealthChanged;
	mutable FLeeAttributeEvent OnStaminaChanged;
	mutable FLeeAttributeEvent OnMaxHealthChanged;
	mutable FLeeAttributeEvent OnMaxStaminaChanged;
	// Delegate to broadcast when the health attribute reaches zero
	mutable FLeeAttributeEvent OnOutOfHealth;

	// Used to track when the health reaches 0.
	bool bOutOfHealth;

	// Store the health before any changes 
	float MaxHealthBeforeAttributeChange;
	float HealthBeforeAttributeChange;

	// Store the health before any changes 
	float MaxStaminaBeforeAttributeChange;
	float StaminaBeforeAttributeChange;

protected:
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

};
