// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePlayerController.h"

#include "LeePlayerState.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/ASavePoint/LeeSavePointSubsystem.h"
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

// [서버] 부활 요청을 SavePointSubsystem에 전달 (사망 대기 중인지 검사는 Subsystem이 담당)
void ALeePlayerController::ServerRequestRespawn_Implementation()
{
	if (ULeeSavePointSubsystem* SavePointSubsystem = GetWorld()->GetSubsystem<ULeeSavePointSubsystem>())
	{
		SavePointSubsystem->RequestRespawn(this);
	}
}
