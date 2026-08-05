// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConsiderationBase.h"
#include "Considerations/StateTreeCommonConsiderations.h"
#include "GameplayTagContainer.h"
#include "LeeStateTreeConsiderations_Combat.generated.h"

/**
 * FLeeConsideration_TimeSinceLastUse의 StateTree 에디터 노출 데이터.
 * 값 소스는 ULeeAttackerProfileComponent::GetTimeSinceLastUse (Q8 결정안 (b) —
 * 강공격에 GAS Cooldown GE가 없어, 실제 게임플레이 제한과 무관한 순수 AI 행동 편향으로 직접 기록·조회한다).
 */
USTRUCT()
struct FLeeTimeSinceLastUseConsiderationInstanceData
{
	GENERATED_BODY()

	/** 사용 기록을 조회할 Enemy Pawn (Context Actor에 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> RequesterPawn = nullptr;

	/** ULeeAttackerProfileComponent에 기록되는 것과 동일한 공격 태그 (예: HeavyAttack.QuotaTag) */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag AttackTag;

	/** 경과 시간(초)을 0~1로 정규화할 범위. TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, DisplayName = "InputRange", Category = "Parameter")
	FFloatInterval Interval = FFloatInterval(0.f, 10.f);
};

/**
 * 마지막 사용 후 경과 시간을 InputRange로 정규화한 뒤 ResponseCurve로 점수를 산출하는 Consideration.
 * 강공격처럼 "너무 자주 나오지 않았으면 좋겠는" 패턴의 Utility 가중치로 사용한다
 * (한 번도 안 썼으면 Interval.Max로 취급해 최댓값을 준다).
 */
USTRUCT(meta = (DisplayName = "Lee Time Since Last Use", Category = "Lee|AI|Consideration"))
struct GAS_PROJECT_API FLeeConsideration_TimeSinceLastUse : public FStateTreeConsiderationCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeTimeSinceLastUseConsiderationInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

protected:
	virtual float GetScore(FStateTreeExecutionContext& Context) const override;

public:
	UPROPERTY(EditAnywhere, Category = "Default")
	FStateTreeConsiderationResponseCurve ResponseCurve;
};

/** FLeeConsideration_TargetHasTag의 StateTree 에디터 노출 데이터 */
USTRUCT()
struct FLeeTargetHasTagConsiderationInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** 확인할 태그 — 그로기/가드중/공격중 등, 하드코딩하지 않고 에셋 조립 시 지정 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag TargetTag;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScoreWhenPresent = 1.f;

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScoreWhenAbsent = 0.f;
};

/** 타겟 ASC가 TargetTag를 보유하고 있으면 ScoreWhenPresent, 아니면 ScoreWhenAbsent를 반환 (그로기/가드중/공격중 가중치용) */
USTRUCT(meta = (DisplayName = "Lee Target Has Tag", Category = "Lee|AI|Consideration"))
struct GAS_PROJECT_API FLeeConsideration_TargetHasTag : public FStateTreeConsiderationCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeTargetHasTagConsiderationInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

protected:
	virtual float GetScore(FStateTreeExecutionContext& Context) const override;
};
