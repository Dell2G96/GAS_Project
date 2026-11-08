// 26.11.05 신규 - Stone 보스 AI (Behavior Tree + StateTree 혼용)

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeNodes_Guard.h"
#include "LeeStateTreeNodes_Boss.generated.h"

class AAIController;

/** 보스의 가드 전환에 필요한 거리, 방향, 현재 타겟을 받는다. */
USTRUCT()
struct FLeeBossGuardConditionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category="Input")
	bool bIsCloseTier = false;

	UPROPERTY(EditAnywhere, Category="Input")
	bool bIsTargetInFront = false;

	/** 26.11.08 추가 - 평가기 bShouldGuardNow (플레이어 공격 감지 결과) */
	UPROPERTY(EditAnywhere, Category="Input")
	bool bShouldGuardNow = false;
};

/** 보스가 공격 사이에 정면의 근접 공격을 막을 수 있는지 확인한다. */
USTRUCT(meta=(DisplayName="Lee Boss Can React With Guard", Category="Lee|AI|Condition"))
struct GAS_PROJECT_API FLeeBossCanReactWithGuard : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FLeeBossGuardConditionData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/**
 * 26.11.08 수정 - 공용 반응 가드(FLeeStateTreeTask_ReactiveGuard)로 옮겼다.
 * 이전에 만든 보스 StateTree 에셋이 깨지지 않도록 이름만 남겨 둔다. 새 트리는 공용 Task를 쓴다.
 */
USTRUCT(meta=(DisplayName="Lee Boss Reactive Guard (구버전)", Category="Lee|AI|Action"))
struct GAS_PROJECT_API FLeeBossReactiveGuard : public FLeeStateTreeTask_ReactiveGuard
{
	GENERATED_BODY()
};

/** FLeeStateTreeCondition_DistanceInRange 데이터 */
USTRUCT()
struct FLeeDistanceInRangeInstanceData
{
	GENERATED_BODY()

	/** 대개 Lee Combat Context의 DistanceToTarget(표면-표면 거리)에 바인딩 */
	UPROPERTY(EditAnywhere, Category = "Input")
	float Distance = 0.f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float MinDistance = 0.f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float MaxDistance = 300.f;
};

/** 거리가 [MinDistance, MaxDistance] 안인지 — 보스 패턴별 사거리 판정 */
USTRUCT(meta = (DisplayName = "Lee Distance In Range", Category = "Lee|AI|Condition"))
struct GAS_PROJECT_API FLeeStateTreeCondition_DistanceInRange : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeDistanceInRangeInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/** FLeeStateTreeTask_BossRecovery 데이터 */
USTRUCT()
struct FLeeBossRecoveryInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float MinDuration = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float MaxDuration = 0.9f;

	/** 후딜 동안 타겟을 계속 바라본다 (다음 패턴 진입이 자연스러워진다) */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bKeepFocusOnTarget = true;

	UPROPERTY()
	float RemainingTime = 0.f;
};

/**
 * 공격 후딜(플레이어 반격 타이밍). Min~Max 사이 랜덤 시간 동안 대기 후 성공.
 * 소울라이크 공정성: 큰 공격일수록 후딜을 길게 줘서 "읽고 때릴 수 있는" 틈을 만든다.
 */
USTRUCT(meta = (DisplayName = "Lee Boss Recovery", Category = "Lee|AI|Action"))
struct GAS_PROJECT_API FLeeStateTreeTask_BossRecovery : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeBossRecoveryInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
