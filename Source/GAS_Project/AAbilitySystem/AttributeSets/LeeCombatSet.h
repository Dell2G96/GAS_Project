// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "LeeAttributeSet.h"
#include "LeeCombatSet.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class GAS_PROJECT_API ULeeCombatSet : public ULeeAttributeSet
{
	GENERATED_BODY()
public:
	ULeeCombatSet();

	ATTRIBUTE_ACCESSORS(ULeeCombatSet, BaseHeal);

	UPROPERTY(BlueprintReadOnly, Category="Lee|Combat")
	FGameplayAttributeData BaseHeal;
};
