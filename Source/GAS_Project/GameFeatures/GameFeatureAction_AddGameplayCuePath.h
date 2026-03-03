// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "UObject/Object.h"
#include "GameFeatureAction_AddGameplayCuePath.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UGameFeatureAction_AddGameplayCuePath : public UGameFeatureAction
{
	GENERATED_BODY()

public:
	UGameFeatureAction_AddGameplayCuePath();
	
	UPROPERTY(EditAnywhere, Category="GameFeature|GameplayCue")
	TArray<FDirectoryPath> DirectoryPathsToAdd;
};
