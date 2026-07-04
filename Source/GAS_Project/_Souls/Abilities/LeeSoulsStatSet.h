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



	UPROPERTY(BlueprintReadOnly, Category = "Lee|SoulsStats")
	FGameplayAttributeData Health;


	UPROPERTY(BlueprintReadOnly, Category = "Lee|SoulsStats")
	FGameplayAttributeData MaxHealth;


	UPROPERTY(BlueprintReadOnly, Category = "Lee|SoulsStats")
	FGameplayAttributeData Stamina;


	UPROPERTY(BlueprintReadOnly, Category = "Lee|SoulsStats")
	FGameplayAttributeData MaxStamina;

	mutable FLeeAttributeEvent OnHealthChanged;
	mutable FLeeAttributeEvent OnStaminaChanged;
	mutable FLeeAttributeEvent OnMaxHealthChanged;
	mutable FLeeAttributeEvent OnMaxStaminaChanged;
	
	// Delegate to broadcast when the health attribute reaches zero
	mutable FLeeAttributeEvent OnOutOfHealth;

	// [신규] 스태미나가 0에 도달했을 때 브로드캐스트 — LeeFinisherTargetComponent가 수신하여 그로기 GE 적용
	mutable FLeeAttributeEvent OnOutOfStamina;

	// Used to track when the health reaches 0.
	bool bOutOfHealth;

	// [신규] 스태미나 0 도달 여부 추적 (중복 브로드캐스트 방지)
	bool bOutOfStamina;

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
