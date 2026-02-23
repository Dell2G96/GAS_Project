// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LeeExperienceDefinition.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULeeExperienceDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, Category="Gameplay")
	TObjectPtr<class ULeePawnData> DefaultPawnData;

	UPROPERTY(EditDefaultsOnly, Category="Gameplay")
	TArray<FString> GameFeaturesToEnable;
};
