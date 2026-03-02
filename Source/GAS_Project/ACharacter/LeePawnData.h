// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LeePawnData.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeePawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULeePawnData(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// 어떤 클래스를 사용할지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|Pawn")
	TSubclassOf<APawn> PawnClass;

	// 어떤 카메라 모드를 사용할지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,Category="Lee|Camera")
	TSubclassOf<class ULeeCameraMode> DefaultCameraMode;

	// 어떤 Input Config 를 사용하지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|InputConfig")
	TObjectPtr<class ULeeInputConfig> InputConfig;

	// 해당 Pawn의 Ability System에 허용할 AbilitySet
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|Abilities")
	TArray<TObjectPtr<class ULeeAbilitySet>> AbilitySets;
};
