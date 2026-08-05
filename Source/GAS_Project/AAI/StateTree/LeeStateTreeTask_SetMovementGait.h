// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "LeeStateTreeTask_SetMovementGait.generated.h"

/** 걷기/달리기 전환 대상 보폭 */
UENUM()
enum class ELeeMovementGait : uint8
{
	Walk,
	Run
};

/** FLeeStateTreeTask_SetMovementGait의 StateTree 에디터 노출 데이터 */
USTRUCT()
struct FLeeSetMovementGaitInstanceData
{
	GENERATED_BODY()

	/** 속도를 적용할 주체 (Context Actor/Pawn에 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	ELeeMovementGait Gait = ELeeMovementGait::Walk;

	/** TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float WalkSpeed = 200.f;

	/** TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float RunSpeed = 450.f;

	// --- 런타임 상태 ---
	UPROPERTY()
	float CachedMaxWalkSpeed = 0.f;

	UPROPERTY()
	bool bHasCachedSpeed = false;
};

/**
 * State 진입 시 Gait에 맞는 MaxWalkSpeed를 캐시 후 대입하고, State 이탈 시 원래 값으로 복원하는 초경량 Task.
 * `FStateTreeMoveToTask`는 속도를 다루지 않으므로 별도 Task로 분리했다.
 * 캐시 후 대입/복원 방식은 이 프로젝트의 기존 관례(ULeeTargetLockComponent::CacheAndApplyMovementFlags /
 * RestoreMovementFlags, CTargetLock_Ability.cpp)와 동일하다 — MoveSpeed Attribute+GE 신설(Q5 (b)안)은
 * 레거시 파일(CPlayerCharacter/CPlayerController) 수정 금지 규칙 때문에 시스템이 이원화되는 문제가 있어
 * 채택하지 않았다.
 * 완료 판정에는 참여하지 않는다 (같은 State에 함께 배치되는 MoveTo 등 다른 Task가 완료를 결정).
 */
USTRUCT(meta = (DisplayName = "Lee Set Movement Gait", Category = "Lee|AI|Action"))
struct GAS_PROJECT_API FLeeStateTreeTask_SetMovementGait : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeSetMovementGaitInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FLeeStateTreeTask_SetMovementGait();

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
