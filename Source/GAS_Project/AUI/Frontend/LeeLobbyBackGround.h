// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LeeLobbyBackGround.generated.h"

/**
 * 
 */
UCLASS(config=EditorPerProjectUserSettings)
class GAS_PROJECT_API ULeeLobbyBackGround : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<class UWorld> BackGroundLevel;
};
