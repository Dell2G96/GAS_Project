// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePlayerCameraManager.h"

#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"

ALeePlayerCameraManager::ALeePlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultFOV = LEE_CAMERA_DEFAULT_FOV;
	ViewPitchMin = LEE_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = LEE_CAMERA_DEFAULT_PITCH_MAX;

	CameraFadeCollection = TSoftObjectPtr<UMaterialParameterCollection>(
		FSoftObjectPath(TEXT("/SoulsCore/Environment/KoreanTemple/Materials/MPC_KT_CameraFade.MPC_KT_CameraFade")));
}

// 카메라 위치를 갱신한 뒤, 가림 페이드 기준점(플레이어 위치)을 MPC에 기록한다
void ALeePlayerCameraManager::UpdateCamera(float DeltaTime)
{
	Super::UpdateCamera(DeltaTime);
	UpdateCameraFade();
}

// 로컬 플레이어가 보는 대상의 위치를 MPC PlayerPos에 넣고 페이드를 켠다. 대상이 없으면 페이드를 끈다
void ALeePlayerCameraManager::UpdateCameraFade()
{
	// 서버에는 원격 플레이어용 카메라 매니저도 있으므로 로컬 플레이어만 MPC를 쓴다
	if (!PCOwner || !PCOwner->IsLocalController())
	{
		return;
	}

	UMaterialParameterCollection* Collection = CameraFadeCollection.LoadSynchronous();
	if (!Collection)
	{
		return;
	}

	// Pawn이 없으면 보는 대상이 컨트롤러 자신이 되므로 페이드를 끈다
	const AActor* Target = GetViewTarget();
	const bool bHasTarget = Target && Target != PCOwner;
	if (bHasTarget)
	{
		const FVector FadePoint = Target->GetActorLocation() + FVector(0.f, 0.f, CameraFadeHeightOffset);
		UKismetMaterialLibrary::SetVectorParameterValue(this, Collection, TEXT("PlayerPos"), FLinearColor(FadePoint));
	}
	UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TEXT("FadeEnabled"), bHasTarget ? 1.f : 0.f);
}
