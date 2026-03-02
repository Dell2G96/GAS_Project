// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "LeePlayerController.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ALeePlayerController : public AModularPlayerController
{
	GENERATED_BODY()
public:
	ALeePlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

	class ALeePlayerState* GetLeePlayerState() const;
	class ULeeAbilitySystemComponent* GetLeeAbilitySystemComponent() const;
	
};
