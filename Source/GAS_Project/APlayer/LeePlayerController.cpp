// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePlayerController.h"

#include "LeePlayerState.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/ACamera/LeePlayerCameraManager.h"

ALeePlayerController::ALeePlayerController(const FObjectInitializer& ObjectInitializer)
{
	PlayerCameraManagerClass = ALeePlayerCameraManager::StaticClass();
}

void ALeePlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (ULeeAbilitySystemComponent* LeeASC= GetLeeAbilitySystemComponent())
	{
		LeeASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}
	Super::PostProcessInput(DeltaTime, bGamePaused);
}

class ALeePlayerState* ALeePlayerController::GetLeePlayerState() const
{
	return CastChecked<ALeePlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

class ULeeAbilitySystemComponent* ALeePlayerController::GetLeeAbilitySystemComponent() const
{
	const ALeePlayerState* LeePlayerState = GetLeePlayerState();
	return (LeePlayerState ? LeePlayerState->GetLeeAbilitySystemComponent() : nullptr);
}
