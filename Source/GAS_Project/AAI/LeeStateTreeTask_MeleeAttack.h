// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "LeeStateTreeTask_MeleeAttack.generated.h"

class AActor;
class ULeeAbilityEndListener;

/**
 * FLeeStateTreeTask_ActivateAbility의 StateTree 에디터 노출 데이터.
 * Actor는 Context(대개 Pawn)에 바인딩하고, AbilityTag는 State마다
 * 약공격/강공격/원거리공격 태그를 다르게 지정해 같은 Task를 재사용한다.
 * (리팩토링 이력: 기존 FLeeStateTreeTask_MeleeAttackInstanceData에 SpecHandle 기반
 *  추적 필드와 Timeout/Grace/fallback 옵션을 추가하며 근접/원거리 공용으로 일반화했다.)
 */
USTRUCT()
struct FLeeActivateAbilityInstanceData
{
	GENERATED_BODY()

	/** 공격을 실행할 주체 (StateTree 에디터에서 Context Actor/Pawn에 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** 활성화할 어빌리티 태그 (예: Souls.Abilities.Attack.Melee.Light/.Heavy 또는 원거리 태그). ASC의 활성화 가능 어빌리티 중 정확히 1개와 매칭되어야 한다 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag AbilityTag;

	/** 어빌리티가 이 시간 안에 끝나지 않으면 Failed 처리. TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float TimeoutSeconds = 8.f;

	/** 태그 폴링 fallback을 시작하기까지의 유예 시간 (초 단위, 틱 수 아님). TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float GraceSeconds = 0.15f;

	/** SpecHandle 리스너를 놓쳤을 경우를 대비한 태그 폴링(Status_Attack_Attacking) fallback 사용 여부 — 기존 구현 방식, 회귀 방지용으로 남겨둔다 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bUseTagPollingFallback = true;

	// --- 런타임 상태 ---
	UPROPERTY()
	FGameplayAbilitySpecHandle ActivatedSpecHandle;

	UPROPERTY()
	float ElapsedSeconds = 0.f;

	/** 활성화한 SpecHandle의 종료만 정확히 감지하는 리스너 (수명이 안정적인 UObject) */
	UPROPERTY()
	TObjectPtr<ULeeAbilityEndListener> EndListener = nullptr;
};

/**
 * 지정된 태그의 어빌리티를 활성화하고 SpecHandle 기반으로 종료를 감지하는 StateTree Task.
 * 근접/원거리 공용 — 약/강/원거리 "선택"은 이 Task를 호출하는 StateTree State(Utility 랜덤
 * Selector 등)가 담당하며, 이 Task는 "태그로 정확히 1개 어빌리티 활성화 + 완료 대기"라는
 * 단일 책임만 가진다.
 *
 * 종료 감지는 ULeeAbilityEndListener(ASC::OnAbilityEnded + SpecHandle 일치 검증)가 1차,
 * 기존 방식이던 Status_Attack_Attacking 태그 폴링은 GraceSeconds 이후 fallback으로만 남겼다.
 */
USTRUCT(meta = (DisplayName = "Lee Activate Ability", Category = "Lee|AI|Action"))
struct GAS_PROJECT_API FLeeStateTreeTask_ActivateAbility : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeActivateAbilityInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FLeeStateTreeTask_ActivateAbility();

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
