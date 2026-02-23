 // Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LeeUserFacingExperience.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeUserFacingExperience : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Experience", meta=(AllowedTypes="Map"))
	FPrimaryAssetId MapID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Experience", meta=(AllowedTypes="LeeExperienceDefinition"))
	FPrimaryAssetId ExperienceID;
};
