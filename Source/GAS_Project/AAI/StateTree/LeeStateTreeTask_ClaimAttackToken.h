// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GAS_Project/AAI/Token/LeeAttackTokenComponent.h"
#include "LeeStateTreeTask_ClaimAttackToken.generated.h"

/**
 * FLeeStateTreeTask_ClaimAttackToken의 StateTree 에디터 노출 데이터.
 * TokenComponent가 바인딩되지 않으면(타겟에 컴포넌트가 없음) 예약 없이 Running을 유지한다.
 */
USTRUCT()
struct FLeeClaimAttackTokenInstanceData
{
	GENERATED_BODY()

	/** 토큰을 예약할 주체 (Enemy Pawn) */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> RequesterPawn = nullptr;

	/** 타겟의 어택 토큰 컴포넌트 (Evaluator 출력 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<ULeeAttackTokenComponent> TokenComponent = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FLeeAttackReservationConfig Reservation;

	/** 런타임 발급 정보 — EnterState에서 채우고 ExitState에서 반납한다 */
	UPROPERTY()
	FLeeAttackClaimHandle Handle;
};

/**
 * 어택 토큰을 EnterState에서 예약하고, State가 어떻게 끝나든(성공/실패/외부 전이) ExitState에서
 * 반드시 반납하는 Task. 이 Task 자체는 State 완료 판정에 참여하지 않는다
 * (생성자에서 bConsideredForCompletion=false — 완료는 같은 State의 Ability 활성화 Task가 결정).
 * 자격 판정(Condition)과 원자적 예약(이 Task)을 분리한다 — TOCTOU를 피하기 위해 실제 점유는
 * 항상 EnterState에서 한 번만 수행한다.
 */
USTRUCT(meta = (DisplayName = "Lee Claim Attack Token", Category = "Lee|AI|Action"))
struct GAS_PROJECT_API FLeeStateTreeTask_ClaimAttackToken : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeClaimAttackTokenInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FLeeStateTreeTask_ClaimAttackToken();

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	/** 예약 성사 시 Enemy의 AttackerProfile에 사용 시각을 기록 (Lee Time Since Last Use Consideration의 데이터 소스) */
	void RecordUseOnProfile(const FInstanceDataType& Data) const;
};
