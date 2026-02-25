// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePlayerCameraManager.h"

ALeePlayerCameraManager::ALeePlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultFOV = LEE_CAMERA_DEFAULT_FOV;
	ViewPitchMin = LEE_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = LEE_CAMERA_DEFAULT_PITCH_MAX;
}
