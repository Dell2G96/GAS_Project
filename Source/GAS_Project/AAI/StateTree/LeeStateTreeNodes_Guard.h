// 26.11.08 신규 - Enemy 공용 반응 가드 StateTree 노드 (보스·일반 적·방패 적 공용)

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "GAS_Project/AAI/StateTree/LeeStateTreeTask_MaintainGuard.h"
#include "LeeStateTreeNodes_Guard.generated.h"

/** FLeeStateTreeCondition_CanReactWithGuard 데이터 — 평가기(Lee Combat Context) 출력에 바인딩한다 */
USTRUCT()
struct FLeeCanReactWithGuardInstanceData
{
	GENERATED_BODY()

	/** 자기 자신 (Context Actor 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** 평가기 bShouldGuardNow */
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bShouldGuardNow = false;

	/** 평가기 bIsTargetInFront */
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bIsTargetInFront = false;
};

/**
 * 지금 플레이어 공격에 반응해 가드할 수 있는지 판단한다.
 * 감지 결과(bShouldGuardNow) + 타깃 정면 + 공격 중·그로기 아님 + 스태미나 > 0
 */
USTRUCT(meta = (DisplayName = "Lee Can React With Guard", Category = "Lee|AI|Condition"))
struct GAS_PROJECT_API FLeeStateTreeCondition_CanReactWithGuard : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeCanReactWithGuardInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/** FLeeStateTreeTask_ReactiveGuard 데이터 — MaintainGuard 데이터 + 위협 진행 여부 */
USTRUCT()
struct FLeeReactiveGuardInstanceData : public FLeeMaintainGuardInstanceData
{
	GENERATED_BODY()

	/** 평가기 bGuardThreatActive — 거짓이 되면 가드를 내린다 */
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bGuardThreatActive = false;
};

/**
 * 플레이어 공격이 끝날 때까지 제자리에서 가드한다.
 * 공격 종료, 가드 깨짐, 퍼펙트 가드 성공(반격 윈도우)이면 끝난다.
 */
USTRUCT(meta = (DisplayName = "Lee Reactive Guard", Category = "Lee|AI|Action"))
struct GAS_PROJECT_API FLeeStateTreeTask_ReactiveGuard : public FLeeStateTreeTask_MaintainGuard
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeReactiveGuardInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

/** FLeeStateTreeTask_ShieldGuard 데이터 — MaintainGuard 데이터 + 퍼펙트 갱신 신호 */
USTRUCT()
struct FLeeShieldGuardInstanceData : public FLeeMaintainGuardInstanceData
{
	GENERATED_BODY()

	/** 평가기 bShouldRefreshPerfect — 참인 틱에 퍼펙트 판정 시간을 다시 연다 */
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bShouldRefreshPerfect = false;
};

/**
 * 방패 적 전용: 이동 Task와 함께 두고 가드를 계속 든다.
 * 가드가 풀리면 스태미나가 있을 때 다시 들고, 타격 직전 신호가 오면 퍼펙트 판정 시간을 다시 연다.
 * 스스로 끝나지 않는다(같은 State의 다른 Task가 완료를 결정).
 */
USTRUCT(meta = (DisplayName = "Lee Shield Guard", Category = "Lee|AI|Action"))
struct GAS_PROJECT_API FLeeStateTreeTask_ShieldGuard : public FLeeStateTreeTask_MaintainGuard
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeShieldGuardInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FLeeStateTreeTask_ShieldGuard();

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
