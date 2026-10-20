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

	// --- 런타임 상태 ---
	UPROPERTY()
	float ElapsedSeconds = 0.f;

	/** 이 Task가 직접 발급한 이동 요청 ID — ExitState에서 이 요청만 중단한다 */
	UPROPERTY()
	FAIRequestID MoveRequestId = FAIRequestID::InvalidRequest;
};

/**
 * EQS를 한 번 실행해 가장 점수가 높은 지점을 구하고, 타겟을 바라본 채(Focus 고정) 그 지점으로 걸어가는 Task.
 * 도착/시간 초과 시 Succeeded, EQS 결과가 없거나 이동 요청이 실패하면 Failed.
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
