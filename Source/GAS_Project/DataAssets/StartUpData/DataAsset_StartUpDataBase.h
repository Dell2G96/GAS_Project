// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DataAsset_StartUpDataBase.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UDataAsset_StartUpDataBase : public UDataAsset
{
	GENERATED_BODY()

	virtual void GiveToAbilitySystemComponent(class UCAbilitySystemComponent* InASCToGive, int32 ApplyLevel = 1);

protected:
	UPROPERTY(EditDefaultsOnly, Category="A|StartUpData")
	TArray<TSubclassOf<class UCGameplayAbility>> ActivateOnGivenAbilities;

	UPROPERTY(EditDefaultsOnly, Category="A|StartUpData")
	TArray<TSubclassOf<class UCGameplayAbility>> ReactiveAbilities;

	void GrantAbilities(const TArray<TSubclassOf<UCGameplayAbility>>& InAbilitiesToGrant, UCAbilitySystemComponent* InASCToGive, int32 ApplyLevel = 1);
};
