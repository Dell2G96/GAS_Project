// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "LeeStateTreeTask_MeleeAttack.generated.h"

class AActor;

/**
 * FLeeStateTreeTask_MeleeAttack의 StateTree 에디터 노출 데이터.
 * Actor는 Context(대개 Pawn)에 바인딩하고, AttackAbilityTag는 State마다
 * 약공격/강공격 태그를 다르게 지정해 같은 Task를 재사용한다.
 */
USTRUCT()
struct FLeeStateTreeTask_MeleeAttackInstanceData
{
	GENERATED_BODY()

	/** 공격을 실행할 주체 (StateTree 에디터에서 Context Actor/Pawn에 바인딩) */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** 활성화할 공격 어빌리티 태그 (예: Souls.Abilities.Attack.Melee.Light 또는 .Heavy) */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag AttackAbilityTag;
};

/**
 * 지정된 태그의 근접 공격 어빌리티를 활성화하고 종료까지 대기하는 StateTree Task.
 * 약/강 "선택"은 이 Task를 호출하는 StateTree State(Utility 랜덤 Selector 등)가 담당하며,
 * 이 Task는 "태그 활성화 + 완료 대기"라는 단일 책임만 가진다.
 */
USTRUCT(meta = (DisplayName = "Lee Melee Attack", Category = "Lee|AI|Action"))
struct GAS_PROJECT_API FLeeStateTreeTask_MeleeAttack : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeStateTreeTask_MeleeAttackInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	FLeeStateTreeTask_MeleeAttack();

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
