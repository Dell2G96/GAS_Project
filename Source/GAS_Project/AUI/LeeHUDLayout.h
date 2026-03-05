// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeActivatableWidget.h"
#include "LeeHUDLayout.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, meta=(DisplayName="Lee HUD Layout"), Category="Lee|HUD")
class GAS_PROJECT_API ULeeHUDLayout : public ULeeActivatableWidget
{
	GENERATED_BODY()
public:
	ULeeHUDLayout(const FObjectInitializer& ObjectInitializer);
};
