// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CWeapon.h"
#include "GameplayAbilitySpecHandle.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "CPlayerWeapon.generated.h"

UCLASS()
class GAS_PROJECT_API ACPlayerWeapon : public ACWeapon
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void AssignGrantedAbilitySpecHandles(const TArray<FGameplayAbilitySpecHandle>& InSpecHandles);

	UFUNCTION(BlueprintPure)
	TArray<FGameplayAbilitySpecHandle> GetGrantedAbilitySpecHandles() const;

private:
	TArray<FGameplayAbilitySpecHandle> GrantedAbilitySpecHandles;
};
