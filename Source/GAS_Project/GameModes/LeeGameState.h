// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LeeGameState.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ALeeGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ALeeGameState();

public:
	UPROPERTY()
	TObjectPtr<class ULeeExperienceManagerComponent> ExperienceManagerComponent;
};
