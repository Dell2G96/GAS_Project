// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueManager.h"
#include "LeeGameplayCueManager.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayCueManager : public UGameplayCueManager
{
	GENERATED_BODY()
public:
	static ULeeGameplayCueManager* Get();

	ULeeGameplayCueManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void RefreshGameplayCuePrimaryAsset();
};
