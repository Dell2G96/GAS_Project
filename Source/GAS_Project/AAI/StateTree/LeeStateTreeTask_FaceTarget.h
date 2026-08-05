// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "LeeStateTreeTask_FaceTarget.generated.h"

class AAIController;

/** FLeeStateTreeTask_FaceTarget의 StateTree 에디터 노출 데이터 */
USTRUCT()
struct FLeeFaceTargetInstanceData
{
	GENERATED_BODY()

	/** AIController (StateTree 에디터에서 Context에 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	/** 정면 정렬 여부를 계산할 때 기준이 되는 controlled Pawn (Context Actor에 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** 바라볼 대상 (Evaluator의 TargetActor 출력에 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** 정면 정렬로 간주할 각도 허용치 (도). TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float FaceToleranceDeg = 10.f;
};

/**
 * AIController의 Focus를 타겟으로 설정해 정면을 맞추고, 허용 각도 이내로 정렬되면 Succeeded를 반환하는 Task.
 * 원거리 공격(ST_RangeAttack의 [Fire] State)에서 사격 전 조준 정렬 용도로 쓴다.
 */
USTRUCT(meta = (DisplayName = "Lee Face Target", Category = "Lee|AI|Action"))
struct GAS_PROJECT_API FLeeStateTreeTask_FaceTarget : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeFaceTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FLeeStateTreeTask_FaceTarget();

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
