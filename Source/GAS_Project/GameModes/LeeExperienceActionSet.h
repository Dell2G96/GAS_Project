// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LeeExperienceActionSet.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, NotBlueprintable)
class GAS_PROJECT_API ULeeExperienceActionSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULeeExperienceActionSet();
	
	//UObject interface
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//End of UObject interface

	
	//UPrimaryDataAsset interface
#if WITH_EDITORONLY_DATA
	virtual void UpdateAssetBundleData() override;
#endif

public:
	//UPrimaryDataAsset interface
	UPROPERTY(EditAnywhere, Category="Actions To Perform")
	TArray<TObjectPtr<class UGameFeatureAction>> Actions;

	// List of Game Feature Plugins this experience wants to have active
	UPROPERTY(EditAnywhere, Category="Feature Dependencies")
	TArray<FString> GameFeaturesToEnable;
};
