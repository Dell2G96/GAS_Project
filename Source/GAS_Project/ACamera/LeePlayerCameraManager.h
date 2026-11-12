// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "LeePlayerCameraManager.generated.h"

class UMaterialParameterCollection;

/**
 *
 */
#define LEE_CAMERA_DEFAULT_FOV (80.0f)
#define LEE_CAMERA_DEFAULT_PITCH_MIN (-89.0f)
#define LEE_CAMERA_DEFAULT_PITCH_MAX (89.0f)


// Lee 카메라 매니저: 기본 FOV·피치 제한을 정하고,
// 카메라와 플레이어 사이를 가리는 환경 메시를 디더로 비우도록 매 프레임 플레이어 위치를 MPC에 기록한다.
UCLASS()
class GAS_PROJECT_API ALeePlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
public:
	ALeePlayerCameraManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void UpdateCamera(float DeltaTime) override;

protected:
	// 카메라 가림 페이드용 MPC (PlayerPos, FadeEnabled, FadeRadius). 사찰 머티리얼(M_KT_*)이 읽는다
	UPROPERTY(EditDefaultsOnly, Category = "Lee|CameraFade")
	TSoftObjectPtr<UMaterialParameterCollection> CameraFadeCollection;

	// 보는 대상 액터 위치(캡슐 중심)에서 페이드 기준점까지 올릴 높이
	UPROPERTY(EditDefaultsOnly, Category = "Lee|CameraFade")
	float CameraFadeHeightOffset = 40.f;

private:
	void UpdateCameraFade();
};
