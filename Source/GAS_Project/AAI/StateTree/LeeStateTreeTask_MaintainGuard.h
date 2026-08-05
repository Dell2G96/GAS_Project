// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "LeeStateTreeTask_MaintainGuard.generated.h"

class AAIController;

/** FLeeStateTreeTask_MaintainGuard의 StateTree 에디터 노출 데이터 */
USTRUCT()
struct FLeeMaintainGuardInstanceData
{
	GENERATED_BODY()

	/** AIController (Context 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	/** 가드 어빌리티를 활성화할 주체 (Context Actor/Pawn에 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** Focus 유지 대상 (Evaluator의 TargetActor 출력에 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float MinDuration = 1.5f;

	/** TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float MaxDuration = 4.f;

	// --- 런타임 상태 ---
	UPROPERTY()
	float ElapsedSeconds = 0.f;

	/** EnterState에서 [MinDuration, MaxDuration] 사이로 무작위 산출 */
	UPROPERTY()
	float TargetDuration = 0.f;

	UPROPERTY()
	FGameplayAbilitySpecHandle ActivatedSpecHandle;
};

/**
 * 기존 ULeeGameplayAbility_Guard를 활성화해 무작위 시간 동안 가드를 유지하는 Task.
 * 가드 로직 자체(스태미나 소모, 퍼펙트 가드 윈도우, 가드 각도)는 전부 기존 어빌리티/
 * ULeeDefenseComponent가 처리하므로, 이 Task는 활성화·Focus 유지·지속시간 판정만 담당한다.
 * GuardStrafe = 이 Task + FLeeStateTreeTask_StrafeAroundTarget을 같은 State에 병렬 배치 (별도 클래스 없음).
 */
USTRUCT(meta = (DisplayName = "Lee Maintain Guard", Category = "Lee|AI|Action"))
struct GAS_PROJECT_API FLeeStateTreeTask_MaintainGuard : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeMaintainGuardInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FLeeStateTreeTask_MaintainGuard();

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
