// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeCameraComponent.h"

#include "LeeCameraMode.h"

ULeeCameraComponent::ULeeCameraComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer), CameraModeStatck(nullptr)
{
}

void ULeeCameraComponent::OnRegister()
{
	Super::OnRegister();

	if (!CameraModeStatck)
	{
		// 초기화
		CameraModeStatck = NewObject<ULeeCameraModeStack>(this);
	}
}

void ULeeCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	check(CameraModeStatck);
	UpdateCameraModes();
}

void ULeeCameraComponent::UpdateCameraModes()
{
	check(CameraModeStatck)
	if (DetermineCameraModeDelegate.IsBound())
	{
		if (const TSubclassOf<ULeeCameraMode> CameraMode = DetermineCameraModeDelegate.Execute())
		{
			// CameraModeStack->PushCameraMode(CameraMode);
		}
	}
}


