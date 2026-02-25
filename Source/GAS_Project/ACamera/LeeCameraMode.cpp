// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeCameraMode.h"

#include "LeeCameraComponent.h"
#include "LeePlayerCameraManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LeeCameraMode)

FLeeCameraModeView::FLeeCameraModeView()
	: Location(ForceInit)
	, Rotation(ForceInit)
	, ControlRotation(ForceInit)
	, FieldOfView(LEE_CAMERA_DEFAULT_FOV)
{
	
}

void FLeeCameraModeView::Blend(const FLeeCameraModeView& Other, float OtherWeight)
{
	if (OtherWeight <= 0.0f)
	{
		return;
	}

	else if (OtherWeight >= 1.0f)
	{
		*this = Other;
		return;
	}

	Location = FMath::Lerp(Location, Other.Location, OtherWeight);

	const FRotator DeltaRotation = (Other.Rotation - Rotation).GetNormalized();
	Rotation += DeltaRotation * OtherWeight;

	const FRotator DeltaControlRotation = (Other.ControlRotation - ControlRotation).GetNormalized();
	ControlRotation += DeltaControlRotation * OtherWeight;

	FieldOfView = FMath::Lerp(FieldOfView, Other.FieldOfView, OtherWeight);
	
}

////////////////////////////////////////////////////////////////////////////
//							 ULeeCameraMode                               //
////////////////////////////////////////////////////////////////////////////

ULeeCameraMode::ULeeCameraMode(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	FieldOfView =  LEE_CAMERA_DEFAULT_FOV;
	ViewPitchMin = LEE_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = LEE_CAMERA_DEFAULT_PITCH_MAX;

	BlendTime = 0.0f;
	BlendAlpha = 1.0f;
	BlendWeight = 1.0f;

	BlendFunction = ELeeCameraModeBlendFunction::EaseOut;
	BlendExponent = 4.0f;
}


void ULeeCameraMode::UpdateCameraMode(float DeltaTime)
{
	// 액터를 활용하여 Pivot[Location|Rotaion]을 계산하여 view 를 업데이트
	UpdateView(DeltaTime);

	UpdateBlending(DeltaTime);
}

void ULeeCameraMode::UpdateView(float DeltaTime)
{
	FVector PivotLocation = GetPivotLocation();
	FRotator PivotRotation = GetPivotRotation();

	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);

	View.Location = PivotLocation;
	View.Rotation = PivotRotation;

	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;
}

FVector ULeeCameraMode::GetPivotLocation() const
{
	const AActor* TargetActor = GetTargetActor();
	check(TargetActor);

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		
		return TargetPawn->GetPawnViewLocation();
	}
	return TargetActor->GetActorLocation();
}

FRotator ULeeCameraMode::GetPivotRotation() const
{
	const AActor* TargetActor = GetTargetActor();
	check(TargetActor);

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		
		return TargetPawn->GetViewRotation();
	}
	return TargetActor->GetActorRotation();
	
}

void ULeeCameraMode::UpdateBlending(float DeltaTime)
{
	if (BlendTime > 0.f)
	{
		BlendAlpha += DeltaTime / BlendTime;
	}
	else
	{
		BlendAlpha = 1.f;
	}
	const float Exponent = (BlendExponent > 0.0f) ? BlendExponent : 1.0f;
	switch (BlendFunction)
	{
	case ELeeCameraModeBlendFunction::Linear:
		BlendWeight = BlendAlpha;
		break;
	case ELeeCameraModeBlendFunction::EaseIn:
		BlendWeight = FMath::InterpEaseIn(0.0f, 1.0f, BlendAlpha, Exponent);
		break;
	case ELeeCameraModeBlendFunction::EaseOut:
		BlendWeight = FMath::InterpEaseOut(0.0f, 1.0f, BlendAlpha, Exponent);
		break;
	case ELeeCameraModeBlendFunction::EaseInOut:
		BlendWeight = FMath::InterpEaseInOut(0.0f, 1.0f, BlendAlpha, Exponent);
		break;
	default:
		checkf(false, TEXT("UpdateBlending: Invalid BlendFunction [%d]\n"), (uint8)BlendFunction);
		break;
	}
}

class ULeeCameraComponent* ULeeCameraMode::GetLeeCameraComponent() const
{
	return CastChecked<ULeeCameraComponent>(GetOuter());
}

AActor* ULeeCameraMode::GetTargetActor() const
{
	const ULeeCameraComponent* LeeCameraComponent = GetLeeCameraComponent();
	return LeeCameraComponent->GetTargetActor();
}



////////////////////////////////////////////////////////////////////////////
//							 GamerModeStack                               //
////////////////////////////////////////////////////////////////////////////

ULeeCameraModeStack::ULeeCameraModeStack(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

ULeeCameraMode* ULeeCameraModeStack::GetCameraModeInstance(TSubclassOf<ULeeCameraMode>& CameraModeClass)
{
	check(CameraModeClass);

	for (ULeeCameraMode* CameraMode : CameraModeInstances)
	{
		if ((CameraMode != nullptr) && (CameraMode->GetClass() == CameraModeClass))
		{
			return CameraMode;
		}
	}
	ULeeCameraMode* NewCameraMode = NewObject<ULeeCameraMode>(GetOuter(), CameraModeClass, NAME_None, RF_NoFlags);
	check(NewCameraMode);

	CameraModeInstances.Add(NewCameraMode);
	return NewCameraMode;
}

void ULeeCameraModeStack::PushCameraMode(TSubclassOf<ULeeCameraMode>& CameraModeClass)
{
	if (!CameraModeClass)
	{
		return;
	}

	ULeeCameraMode* CameraMode = GetCameraModeInstance(CameraModeClass);
	check(CameraMode);

	int32 StackSize = CameraModeStack.Num();
	if ((StackSize > 0) && (CameraModeStack[0] == CameraMode))
	{
		return;
	}
	int32 ExistingStackIndex = INDEX_NONE;
	float ExistingStackContribution = 1.f;

	for (int32 StackIndex = 0; StackIndex < StackSize; ++StackIndex)
	{
		if (CameraModeStack[StackIndex] == CameraMode)
		{
			ExistingStackIndex = StackIndex;
			ExistingStackContribution *= CameraMode->BlendWeight;
			break;
		}
		else
		{
			ExistingStackContribution *= (1.f - CameraModeStack[StackIndex]->BlendWeight);
		}
	}

	if (ExistingStackIndex != INDEX_NONE)
	{
		CameraModeStack.RemoveAt(ExistingStackIndex);
		StackSize--;
	}
	else
	{
		ExistingStackContribution = 0.f;
	}
	const bool bShouldBlend = ((CameraMode->BlendTime > 0.f) && (StackSize > 0));
	const float BlendWeight = (bShouldBlend ? ExistingStackContribution : 1.f);
	CameraMode->BlendWeight = BlendWeight;

	CameraModeStack.Insert(CameraMode, 0);
	CameraModeStack.Last()->BlendWeight = 1.f;
}

void ULeeCameraModeStack::EvaluateStack(float DeltaTime, FLeeCameraModeView& OutCameraModeView)
{
	// 탑 -> 바텀 [0 -> Num] 까지 순차적으로 스택에 있는 카메라모드를 업데이트
	UpdateStack(DeltaTime);

	// 바텀 -> 탑 까지 카메라모드스택에 대해 블랜딩 진행
	BlendStack(OutCameraModeView);
}

void ULeeCameraModeStack::UpdateStack(float DeltaTime)
{
	const int32 StackSize = CameraModeStack.Num();
	if (StackSize <= 0)
	{
		return;
	}

	int32 RemoveCount = 0;
	int32 RemoveIndex = INDEX_NONE;
	for (int32 StackIndex = 0; StackIndex < StackSize; ++StackIndex)
	{
		ULeeCameraMode* CameraMode = CameraModeStack[StackIndex];
		check(CameraMode);

		CameraMode->UpdateCameraMode(DeltaTime);


		// 만약 하나라도 카메라모드가 blendweight 가 1.0에 도달했다면
		// 그 이후 카메라모드를 제거
		if (CameraMode->BlendWeight >= 1.f)
		{
			RemoveIndex = (StackSize +1);
			RemoveCount = (StackSize - RemoveIndex);
			break;
		}
	}

	if (RemoveCount > 0)
	{
		CameraModeStack.RemoveAt(RemoveIndex, RemoveCount);
	}
}

void ULeeCameraModeStack::BlendStack(FLeeCameraModeView& OutCameraModeView) const
{
	const int32 StackSize = CameraModeStack.Num();
	if (StackSize <=0)
	{
		return;
	}

	const ULeeCameraMode* CameraMode = CameraModeStack[StackSize - 1];
	check(CameraMode);

	OutCameraModeView = CameraMode->View;

	for (int32 StackIndex = (StackSize - 2); StackIndex >=0; --StackIndex)
	{
		CameraMode = CameraModeStack[StackIndex];
		check(CameraMode);

		OutCameraModeView.Blend(CameraMode->View, CameraMode->BlendWeight);
	}
}





























