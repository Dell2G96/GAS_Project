// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeCameraMode.h"
#include "LeeCameraMode_Finisher.generated.h"

/**
 * 처형/암살 시네마틱 카메라 모드.
 *
 * 동작:
 *  - 시퀀스 시작 시(스택 Push 순간) 공격자·피해자의 중간점을 앵커로 1회 캐시하고,
 *    이후에는 앵커 기준 고정 프레이밍을 유지한다 (몽타주 중 카메라 흔들림/사망 순간 튐 방지)
 *  - 피해자는 ULeeHeroComponent::GetAbilityCameraFocusTarget()에서 읽는다
 *    (GA_Finisher가 SetAbilityCameraMode() 호출 시 함께 전달)
 *  - View.ControlRotation은 플레이어가 보던 방향을 그대로 통과시킨다 — LeeCameraComponent가
 *    매 프레임 ControlRotation을 PC에 강제 적용하므로, 여기에 시네마틱 방향을 넣으면
 *    피니셔 종료 후 엉뚱한 방향에서 복귀하게 된다. 통과시키면 복원 코드 없이 원래 시점 복귀가 보장된다.
 *
 * 사용: 이 클래스를 부모로 BP(예: CM_Finisher)를 만들어 오프셋/FOV/BlendTime을 튜닝하고,
 *       무기 FinisherData의 AnimSet.CameraMode에 지정한다.
 */
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeCameraMode_Finisher : public ULeeCameraMode
{
	GENERATED_BODY()

public:
	ULeeCameraMode_Finisher(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnPushed() override;
	virtual void UpdateView(float DeltaTime) override;

protected:
	/**
	 * 앵커 로컬 기준 카메라 위치.
	 * 앵커 = 공격자·피해자 중간점, +X = 공격자→피해자 방향.
	 * X = 뒤(-)/앞(+), Y = 측면(0이 아니면 옆에서 잡는 연출), Z = 높이
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Finisher")
	FVector CameraOffset = FVector(-150.0, 180.0, 60.0);

	/** 앵커 로컬 기준 카메라가 바라볼 지점 (가슴 높이 권장) */
	UPROPERTY(EditDefaultsOnly, Category = "Finisher")
	FVector FocusOffset = FVector(0.0, 0.0, 60.0);

	/** 벽 침투 보정용 스윕 구체 반경. 막히면 카메라를 히트 지점까지 당긴다 */
	UPROPERTY(EditDefaultsOnly, Category = "Finisher", meta = (ClampMin = "0.0"))
	float CollisionProbeRadius = 12.0f;

private:
	/** 시퀀스 시작 시 1회 캐시한 앵커 트랜스폼 */
	FTransform CachedAnchor;

	/** 카메라 충돌 스윕에서 무시할 피해자 (캐시 시점에 확정) */
	TWeakObjectPtr<AActor> CachedVictim;

	bool bAnchorCached = false;
};
