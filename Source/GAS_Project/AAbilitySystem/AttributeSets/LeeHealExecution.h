// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "LeeHealExecution.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeHealExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	ULeeHealExecution();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
