// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "LeeAttributeSet.h"
#include "LeeHealthSet.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class GAS_PROJECT_API ULeeHealthSet : public ULeeAttributeSet
{
	GENERATED_BODY()
public:
	ULeeHealthSet();

	ATTRIBUTE_ACCESSORS(ULeeHealthSet, Health);
	ATTRIBUTE_ACCESSORS(ULeeHealthSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(ULeeHealthSet, Healing);

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;


	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;



	/** 현재 체력 */
	UPROPERTY(BlueprintReadOnly, Category="Lee|Health")
	FGameplayAttributeData Health;

	/** 체력 최대치 */
	UPROPERTY(BlueprintReadOnly, Category="Lee|Health")
	FGameplayAttributeData MaxHealth;

	/** 체력 회복치 */
	UPROPERTY(BlueprintReadOnly, Category="Lee|Health")
	FGameplayAttributeData Healing;
};
