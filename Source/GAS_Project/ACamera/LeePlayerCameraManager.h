// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "LeePlayerCameraManager.generated.h"

/**
 * 
 */
#define LEE_CAMERA_DEFAULT_FOV (80.0f)
#define LEE_CAMERA_DEFAULT_PITCH_MIN (-89.0f)
#define LEE_CAMERA_DEFAULT_PITCH_MAX (89.0f)


UCLASS()
class GAS_PROJECT_API ALeePlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
public:
	ALeePlayerCameraManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
