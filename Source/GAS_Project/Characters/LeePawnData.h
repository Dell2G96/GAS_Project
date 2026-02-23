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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|Pawn")
	TSubclassOf<APawn> PlayerPawn;
};
