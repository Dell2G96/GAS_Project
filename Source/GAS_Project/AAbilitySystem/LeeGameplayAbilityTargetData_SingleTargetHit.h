// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "LeeGameplayAbilityTargetData_SingleTargetHit.generated.h"

/**
 * 
 */
USTRUCT()
struct FLeeGameplayAbilityTargetData_SingleTargetHit : public FGameplayAbilityTargetData_SingleTargetHit
{
	GENERATED_BODY()
public:
	FLeeGameplayAbilityTargetData_SingleTargetHit()
		:CartridgeID(-1)
	{}

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FLeeGameplayAbilityTargetData_SingleTargetHit::StaticStruct();
	}

	UPROPERTY()
	int32 CartridgeID;
};
