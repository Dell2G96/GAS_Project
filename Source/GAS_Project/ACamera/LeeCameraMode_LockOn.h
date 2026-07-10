// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeCameraMode.h"
#include "LeeCameraMode_LockOn.generated.h"

/**
 * 타겟 락온 중 사용하는 카메라 모드.
 *
 * 피니셔 카메라(ULeeCameraMode_Finisher)와 정반대 역할을 한다:
 *  - 피니셔 카메라는 View.ControlRotation을 "통과"시켜 플레이어 시점을 보존한다 (연출용, 일시적)
 *  - 락온 카메라는 View.ControlRotation을 타겟 방향으로 "구동"한다 (조작용, 지속적)
 *    → LeeCameraComponent::GetCameraView()가 매 프레임 이 값을 PC에 적용하므로,
 *      Input_Move의 이동 기준과 CharacterMovement의 DesiredRotation이 자동으로 타겟 기준이 되어
 *      스트레이프 이동이 성립한다 (레거시 CTargetLock_Ability가 틱에서 수동으로 하던 일을
 *      카메라 스택 구조가 대체한다).
 *
 * 타겟은 ULeeTargetLockComponent가 소유하며, 이 클래스는 그 상태를 읽기만 한다.
 */
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeCameraMode_LockOn : public ULeeCameraMode
{
	GENERATED_BODY()

public:
	ULeeCameraMode_LockOn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnPushed() override;
	virtual void UpdateView(float DeltaTime) override;

protected:
	/**
	 * 플레이어 피벗 로컬 기준 카메라 위치 (룩앳 회전을 기준으로 회전 적용).
	 * X = 전후 거리(음수 = 뒤), Y = 숄더 오프셋(좌우), Z = 높이.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock")
	FVector CameraOffset = FVector(-420.0, 80.0, 80.0);

	/** 룩앳 피치 클램프 (도) — 절벽 위/아래 타겟에서 과도하게 꺾이는 것 방지 */
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock")
	float PitchMin = -35.0f;

	UPROPERTY(EditDefaultsOnly, Category = "TargetLock")
	float PitchMax = 10.0f;

	/** 룩앳 회전 보간 속도 — 타겟 전환/재락온 시 카메라가 스냅되지 않고 부드럽게 이동 */
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock", meta = (ClampMin = "0.1"))
	float RotationInterpSpeed = 8.0f;

	/** 벽 침투 보정용 스윕 구체 반경 */
	UPROPERTY(EditDefaultsOnly, Category = "TargetLock", meta = (ClampMin = "0.0"))
	float CollisionProbeRadius = 12.0f;

private:
	/** 현재 프레임의 룩앳 회전 (매 프레임 보간되어 누적됨 — OnPushed에서 스냅 방지용으로 초기화) */
	FRotator CurrentRotation = FRotator::ZeroRotator;

	bool bRotationInitialized = false;
};
