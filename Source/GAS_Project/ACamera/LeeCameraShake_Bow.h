// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "LeeCameraShake_Bow.generated.h"

/**
 * 활 차지 중 계속 도는 약한 흔들림 (끝없이 재생).
 * 어빌리티가 ShakeScale을 차지 비율(0~1)로 바꿔 선형으로 세게 만든다.
 */
UCLASS()
class GAS_PROJECT_API ULeeCameraShake_BowCharge : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	ULeeCameraShake_BowCharge(const FObjectInitializer& ObjectInitializer);
};

/**
 * 활 최대 차지 도달(자동 발사) 순간의 큰 흔들림 (짧게 한 번).
 */
UCLASS()
class GAS_PROJECT_API ULeeCameraShake_BowFullCharge : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	ULeeCameraShake_BowFullCharge(const FObjectInitializer& ObjectInitializer);
};
