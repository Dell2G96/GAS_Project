// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "ModularGameState.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class MODULARGAMEPLAYACTORS_API AModularGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
public:
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

};


/** Pair this with a ModularGameState */
UCLASS(MinimalAPI, Blueprintable)
class AModularGameState : public AGameState
{
	GENERATED_BODY()

public:
	//~ Begin AActor interface
	 virtual void PreInitializeComponents() override;
	 virtual void BeginPlay() override;
	 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor interface

protected:
	//~ Begin AGameState interface
	virtual void HandleMatchHasStarted() override;
	//~ Begin AGameState interface
};

#undef UE_API