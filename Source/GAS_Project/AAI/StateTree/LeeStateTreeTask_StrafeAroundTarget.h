// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "AITypes.h"
#include "LeeStateTreeTask_StrafeAroundTarget.generated.h"

class AAIController;

/** FLeeStateTreeTask_StrafeAroundTarget의 StateTree 에디터 노출 데이터 */
USTRUCT()
struct FLeeStrafeAroundTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** 원을 그릴 중심 (Evaluator의 TargetActor 출력에 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float StrafeRadius = 350.f;

	/**
	 * 목표점을 얼마나 앞서 놓을지 계산할 때 쓰는 기준 각속도 (도/초).
	 * 실제 회전 속도는 캐릭터의 이동 속도가 결정하며, 이 값은 "당근"을 두는 거리에만 영향을 준다.
	 * TODO: 튜닝 필요
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float AngularSpeedDeg = 45.f;

	/** 좌/우 방향 전환 평균 주기 (초). 실제로는 ±25% 무작위 편차를 적용한다. TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.1"))
	float DirectionChangeInterval = 3.f;

	/** 목표점 계산은 매 틱 하되, 실제 경로 재요청은 이 주기로만 (매 틱 요청 금지) */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.05"))
	float StrafeGoalUpdateInterval = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVector NavProjectionExtent = FVector(50.f, 50.f, 150.f);

	/** NavMesh 투영 실패 시 반경을 줄여가며 재시도할 횟수 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "1"))
	int32 MaxProjectionAttempts = 3;

	/**
	 * 목표점 도착 판정 반경 (MoveTo의 AcceptanceRadius로 사용).
	 * 목표는 항상 이 값의 2배 이상 앞에 놓이므로, 크게 잡으면 정지 구간이 생긴다.
	 * TODO: 튜닝 필요
	 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float MinArcAdvanceDistance = 30.f;

	/** 유효 반경이 StrafeRadius로 되돌아가는 속도 (cm/초). 공격 직후 붙어 있어도 서서히 간격을 벌린다 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float RadiusRecoverySpeed = 150.f;

	/** 이 횟수만큼 연속으로 NavMesh 투영에 실패해야 Task를 실패 처리한다 (1회 실패로 전투가 끊기지 않게) */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "1"))
	int32 MaxConsecutiveProjectionFailures = 3;

	/** 정방향 투영이 전부 실패하면 반대 방향(180도)으로 재시도할지 여부 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bRetryOppositeOnFail = true;

	/** TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float MinDuration = 1.5f;

	/** TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float MaxDuration = 4.f;

	// --- 런타임 상태 ---
	UPROPERTY()
	float ElapsedSeconds = 0.f;

	UPROPERTY()
	float TargetDuration = 0.f;

	/** 이번 갱신에서 계산한 목표 각도 (도). 디버그 확인용 — 판정에는 매번 실제 위치 각도를 다시 쓴다 */
	UPROPERTY()
	float CurrentAngleDeg = 0.f;

	/** 직전 갱신에서 사용한 유효 반경. StrafeRadius로 서서히 회복시키기 위해 보관한다 */
	UPROPERTY()
	float CurrentRadius = 0.f;

	/** 연속 NavMesh 투영 실패 횟수 */
	UPROPERTY()
	int32 ConsecutiveProjectionFailures = 0;

	/** +1 = 시계 방향, -1 = 반시계 방향 */
	UPROPERTY()
	float DirectionSign = 1.f;

	UPROPERTY()
	float TimeSinceDirectionChange = 0.f;

	UPROPERTY()
	float NextDirectionChangeInterval = 0.f;

	UPROPERTY()
	float TimeSinceLastGoalUpdate = 0.f;

	/** 이 Task가 직접 발급한 이동 요청 ID — ExitState에서 이 요청만 중단한다 */
	UPROPERTY()
	FAIRequestID MoveRequestId = FAIRequestID::InvalidRequest;
};

/**
 * 타겟을 중심으로 반경 StrafeRadius의 원을 따라 좌우로 도는 Task.
 * 목표점(각도) 계산은 매 틱 갱신하되, 실제 NavMesh 경로 재요청은 StrafeGoalUpdateInterval
 * 주기로만 수행해 매 틱 무한 경로 요청을 방지한다 (리뷰 §4-2).
 * GuardStrafe = 이 Task + FLeeStateTreeTask_MaintainGuard를 같은 State에 병렬 배치 (별도 클래스 없음).
 */
USTRUCT(meta = (DisplayName = "Lee Strafe Around Target", Category = "Lee|AI|Action"))
struct GAS_PROJECT_API FLeeStateTreeTask_StrafeAroundTarget : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeStrafeAroundTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FLeeStateTreeTask_StrafeAroundTarget();

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
