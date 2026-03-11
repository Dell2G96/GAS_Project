// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/GameModeBase.h"
#include "ModularGameMode.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class MODULARGAMEPLAYACTORS_API AModularGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	AModularGameModeBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

UCLASS(Blueprintable)
class MODULARGAMEPLAYACTORS_API AModularGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	AModularGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	
};


