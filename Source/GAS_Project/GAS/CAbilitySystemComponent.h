// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "CAbilitySystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API UCAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Ability")
	void GrantHeroWeaponAbilities(const TArray<struct FPlayerAbilitySet>& InAbilitySets,TArray<struct FGameplayAbilitySpecHandle>& OutGrantedAbilitySpecHandles , int32 ApplyLevel = 1);

};
