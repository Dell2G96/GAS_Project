// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LeeHUD.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ALeeHUD : public AHUD
{
	GENERATED_BODY()
public:
	ALeeHUD(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	
};
