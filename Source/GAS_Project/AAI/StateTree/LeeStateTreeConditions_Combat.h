// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "GameplayTagContainer.h"
#include "GAS_Project/AAI/Token/LeeAttackTokenComponent.h"
#include "LeeStateTreeConditions_Combat.generated.h"

class ULeeAttackerProfileComponent;

/**
 * FLeeStateTreeCondition_CanClaimAttackToken의 StateTree 에디터 노출 데이터.
 * TokenComponent가 바인딩되지 않으면(타겟에 컴포넌트가 없으면) 제한 없이 통과시킨다
 * (ULeeAttackTokenComponent 조회 실패 시 정책과 동일, §4-3).
 */
USTRUCT()
struct FLeeCanClaimAttackTokenInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> RequesterPawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<ULeeAttackTokenComponent> TokenComponent = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FLeeAttackReservationConfig Reservation;
};

/** 지정된 한 종류의 공격(Reservation)을 지금 Claim할 수 있는지 예약 없이 확인한다 (예: LightAttack/HeavyAttack/RangedAttack 각각의 Enter Condition) */
USTRUCT(meta = (DisplayName = "Lee Can Claim Attack Token", Category = "Lee|AI|Condition"))
struct GAS_PROJECT_API FLeeStateTreeCondition_CanClaimAttackToken : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeCanClaimAttackTokenInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/**
 * FLeeStateTreeCondition_TimeSinceLastUse의 StateTree 에디터 노출 데이터.
 * 값 소스는 ULeeAttackerProfileComponent::GetTimeSinceLastUse (Consideration과 동일한 데이터 소스).
 */
USTRUCT()
struct FLeeTimeSinceLastUseConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> RequesterPawn = nullptr;

	/** 조회할 공격 식별 태그 (예: Souls.Attacker.Ranged) — ClaimAttackToken Task가 RecordUse로 기록한 것과 같은 태그여야 한다 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag AttackTag;

	/** 이 시간(초) 이상 지나야 다시 쏠 수 있다 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float MinSeconds = 1.0f;
};

/**
 * 지정된 공격을 마지막으로 쓴 지 MinSeconds 이상 지났는지 확인한다.
 * 26.10.19 추가 — RangedAttack에 재발사 간격을 거는 Enter Condition이 없어서
 * 토큰만 잡히면 매 틱 연속 발사되는 문제가 있었다(R7).
 */
USTRUCT(meta = (DisplayName = "Lee Time Since Last Use", Category = "Lee|AI|Condition"))
struct GAS_PROJECT_API FLeeStateTreeCondition_TimeSinceLastUse : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeTimeSinceLastUseConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/**
 * FLeeStateTreeCondition_CanClaimAnyMeleeAttack의 StateTree 에디터 노출 데이터.
 * ProfileComponent가 없으면 애초에 근접 공격 후보가 없으므로 false를 반환한다.
 */
USTRUCT()
struct FLeeCanClaimAnyMeleeAttackInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> RequesterPawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<ULeeAttackTokenComponent> TokenComponent = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<ULeeAttackerProfileComponent> ProfileComponent = nullptr;
};

/** 근접 공격(Light/Heavy) 중 하나라도 지금 Claim 가능하면 true — [MeleeAttack] 부모 State의 Enter Condition용 (리뷰 §3-4) */
USTRUCT(meta = (DisplayName = "Lee Can Claim Any Melee Attack", Category = "Lee|AI|Condition"))
struct GAS_PROJECT_API FLeeStateTreeCondition_CanClaimAnyMeleeAttack : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeCanClaimAnyMeleeAttackInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/**
 * FLeeStateTreeCondition_TargetHasGameplayTag의 StateTree 에디터 노출 데이터.
 * TargetTag는 에셋 조립 시점에 에디터에서 직접 지정한다 — 그로기/사망/가드 등 어떤 실제 태그
 * (레거시 Status::* 또는 Lee Souls::*)를 읽을지는 C++에 하드코딩하지 않고 여기서 결정한다.
 */
USTRUCT()
struct FLeeTargetHasGameplayTagInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag TargetTag;
};

/** 타겟 ASC가 TargetTag를 보유하고 있는지 확인 (내장 Has Tag는 self 기준이라 대체 불가) */
USTRUCT(meta = (DisplayName = "Lee Target Has Gameplay Tag", Category = "Lee|AI|Condition"))
struct GAS_PROJECT_API FLeeStateTreeCondition_TargetHasGameplayTag : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeTargetHasGameplayTagInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/**
 * FLeeStateTreeCondition_SelfHasGameplayTag의 StateTree 에디터 노출 데이터.
 * Actor는 Context(자기 자신 Pawn)에 자동 바인딩된다.
 */
USTRUCT()
struct FLeeSelfHasGameplayTagInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Actor = nullptr;

	/** 검사할 태그 (예: 원거리 공격 쿨다운 태그 Souls.Cooldown.Morgana.Ranged) */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FGameplayTag Tag;

	/** true면 결과를 뒤집는다 — "태그가 없을 때" 통과 (쿨타임이 끝났을 때만 진입) */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvert = false;
};

/**
 * 26.10.21 추가 — 자기 자신 ASC가 Tag를 보유하고 있는지 확인한다.
 * RangedAttack 진입 조건에 쿨다운 태그(bInvert=true)를 걸어, GA 쿨타임 중에는 진입 자체를 막는 데 쓴다.
 */
USTRUCT(meta = (DisplayName = "Lee Self Has Gameplay Tag", Category = "Lee|AI|Condition"))
struct GAS_PROJECT_API FLeeStateTreeCondition_SelfHasGameplayTag : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLeeSelfHasGameplayTagInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
