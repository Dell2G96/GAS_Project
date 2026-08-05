// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "LeeStateTreeEvaluator_CombatContext.generated.h"

class AAIController;
class ULeeAttackTokenComponent;
class ULeeAttackerProfileComponent;

/**
 * FLeeStateTreeEvaluator_CombatContext의 StateTree 에디터 노출 데이터.
 * Input은 Context(AIController/Actor)에 바인딩하고, Output은 각 State의 Enter Condition·
 * Consideration·Task 파라미터가 읽어간다. 타겟 판단 자체는 하지 않고
 * ULeeTargetSelectionComponent가 낸 결론을 그대로 옮긴다.
 */
USTRUCT()
struct FLeeCombatContextInstanceData
{
	GENERATED_BODY()

	// --- Input (Context 바인딩) ---
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	/** controlled Pawn (StateTree 에디터에서 Context Actor에 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	// --- Parameter ---
	/** true면 Z축을 무시하고 수평 거리만 계산 (소울라이크 지형 기본) */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bUse2DDistance = true;

	/** TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float CloseEnterDistance = 300.f;

	/** TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float CloseExitDistance = 350.f;

	/** TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float FarEnterDistance = 900.f;

	/** TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float FarExitDistance = 800.f;

	// --- Output ---
	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<ULeeAttackTokenComponent> AttackTokenComponent = nullptr;

	/** 자신(Enemy Pawn)의 공격 프로필 — Linked Asset 파라미터가 예: AttackerProfileComponent.LightAttack 처럼 프로퍼티 경로로 바로 참조할 수 있도록 출력한다 */
	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<ULeeAttackerProfileComponent> AttackerProfileComponent = nullptr;

	/** 캡슐 반경 보정을 뺀 표면-표면 거리 (cm), 0 미만은 0으로 클램프 */
	UPROPERTY(EditAnywhere, Category = "Output")
	float DistanceToTarget = 0.f;

	/** 자신의 정면 기준 타겟 방향까지의 각도 (도, -180~180) */
	UPROPERTY(EditAnywhere, Category = "Output")
	float AngleToTarget = 0.f;

	/** 타겟이 전방 반구(±90도) 안에 있는가 */
	UPROPERTY(EditAnywhere, Category = "Output")
	bool bIsTargetInFront = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	float SelfStaminaRatio = 1.f;

	UPROPERTY(EditAnywhere, Category = "Output")
	float SelfHealthRatio = 1.f;

	// 히스테리시스 적용된 거리 티어 플래그 (P0-6) — 서로 배타적
	UPROPERTY(EditAnywhere, Category = "Output")
	bool bIsCloseTier = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bIsMidTier = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bIsFarTier = false;

	/** 디버그/애니용 참고 값 — 현재 어택 토큰을 하나도 Claim할 수 없는 상태인지 (StateTree 판정에는 쓰지 않는다, P0-3) */
	UPROPERTY(EditAnywhere, Category = "Output")
	bool bIsAttackSuppressed = false;

	// --- 내부 상태 ---
	/** 타겟 변경 감지용 — 바뀐 프레임에만 AttackTokenComponent를 재해석한다 */
	UPROPERTY()
	TWeakObjectPtr<AActor> CachedTargetForTokenLookup;

	/** 거리 티어 이력 (0=Close, 1=Mid, 2=Far) — 히스테리시스 상태 머신의 현재 상태 */
	UPROPERTY()
	uint8 CurrentTier = 1;
};

/**
 * Enemy StateTree Root에 배치하는 전투 컨텍스트 Evaluator.
 * ULeeTargetSelectionComponent(타겟)·ULeeThreatComponent·ULeeSoulsStatSet(스태미나/체력)의
 * 결론을 매 프레임 모아 StateTree가 바로 읽을 수 있는 거리/각도/티어 값으로 가공한다.
 * 판단은 하지 않는다 — 타겟 선정은 ULeeTargetSelectionComponent, 토큰 가부는
 * ULeeAttackTokenComponent가 전담하고, 이 Evaluator는 그 결과를 옮기고 파생값만 계산한다.
 */
USTRUCT(meta = (DisplayName = "Lee Combat Context", Category = "Lee|AI|Evaluator"))
struct GAS_PROJECT_API FLeeStateTreeEvaluator_CombatContext : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeCombatContextInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
