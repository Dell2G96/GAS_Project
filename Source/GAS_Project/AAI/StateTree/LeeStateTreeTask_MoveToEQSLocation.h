// Fill out your copyright notice in the Description page of Project Settings.
// 26.10.21 신규 작성 — 원거리 공격 쿨타임 동안 EQS로 고른 지점으로 재배치(스트래핑)하는 Task

#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "AITypes.h"
#include "LeeStateTreeTask_MoveToEQSLocation.generated.h"

class AAIController;
class UEnvQuery;

/** FLeeStateTreeTask_MoveToEQSLocation의 StateTree 에디터 노출 데이터 */
USTRUCT()
struct FLeeMoveToEQSLocationInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	/** EQS의 Querier (자기 자신 Pawn) */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** 이동 중 바라볼 대상 (Evaluator의 TargetActor 출력에 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** 이동할 지점을 고를 EQS (예: EQS_FindRangedReposition_Souls) */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<UEnvQuery> QueryTemplate = nullptr;

	/** 도착 판정 반경 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 50.f;

	/** 이 시간이 지나면 도착하지 못해도 성공 처리 — 길이 막혀도 영원히 걷지 않고 다시 공격 판단으로 돌아간다 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.1"))
	float MaxMoveDuration = 4.f;

	/** 26.11.25 추가 - 이동 중 이 주기(초)마다 EQS를 다시 돌려 목표를 갱신한다 (움직이는 플레이어 둘레로 접근할 때). 0이면 처음 한 번만 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float RequeryInterval = 0.f;

	/** 26.11.25 추가 - true면 타겟을 바라본 채 옆/뒤로 걷고, false면 가는 방향을 보고 걷는다 (접근은 false) */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bFaceTargetWhileMoving = true;

	/** 26.11.25 추가 - EQS 결과가 없으면 실패하지 않고 타겟에게 바로 다가간다 (접근용) */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bMoveToTargetOnQueryFail = false;

	// --- 런타임 상태 ---
	UPROPERTY()
	float ElapsedSeconds = 0.f;

	/** 26.11.25 추가 - 마지막 EQS 실행 후 지난 시간 */
	UPROPERTY()
	float TimeSinceQuery = 0.f;

	/** 이 Task가 직접 발급한 이동 요청 ID — ExitState에서 이 요청만 중단한다 */
	UPROPERTY()
	FAIRequestID MoveRequestId = FAIRequestID::InvalidRequest;
};

/**
 * EQS를 실행해 가장 점수가 높은 지점을 구하고, 타겟을 바라본 채(Focus 고정) 그 지점으로 걸어가는 Task.
 * 도착/시간 초과 시 Succeeded, EQS 결과가 없거나 이동 요청이 실패하면 Failed.
 * 26.11.25 추가 - RequeryInterval > 0이면 이동 중 주기적으로 목표를 다시 구한다 (빈 지점으로 접근).
 */
USTRUCT(meta = (DisplayName = "Lee Move To EQS Location", Category = "Lee|AI|Action"))
struct GAS_PROJECT_API FLeeStateTreeTask_MoveToEQSLocation : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeMoveToEQSLocationInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FLeeStateTreeTask_MoveToEQSLocation();

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
