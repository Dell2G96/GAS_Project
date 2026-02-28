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


	// Gameplay 용도에 맞게 분류의 목적으로 사용
	UPROPERTY(EditDefaultsOnly, Category=Gameplay)
	TArray<TObjectPtr<class ULeeExperienceActionSet>> ActionSets;

	// 일반적인 GameFeatureAction의 역할
	UPROPERTY(EditDefaultsOnly,Instanced, Category="Actions")
	TArray<TObjectPtr<class UGameFeatureAction>> Actions;
};
