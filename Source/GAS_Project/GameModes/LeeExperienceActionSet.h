// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LeeExperienceActionSet.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeExperienceActionSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULeeExperienceActionSet();

#if WITH_EDITORONLY_DATA
	virtual void UpdateAssetBundleData() override;
#endif

	UPROPERTY(EditAnywhere, Category="Actions To Perform")
	TArray<TObjectPtr<class UGameFeatureAction>> Actions;
	
};
