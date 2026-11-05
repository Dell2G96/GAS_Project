#pragma once
#include "CoreMinimal.h"
#include "GAS_Project/AAI/LeeStateTreeTask_MeleeAttack.h"
#include "StateTreeConditionBase.h"
#include "LeeBossAttackNodes.generated.h"

class AAIController;

USTRUCT()
struct FLeeBossAttackData : public FLeeActivateAbilityInstanceData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AAIController> AIController = nullptr;
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<AActor> TargetActor = nullptr;
	UPROPERTY(EditAnywhere, Category="Input")
	float Distance = 0.f;
	UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin="0"))
	float MaxDistance = 350.f;
	/** Shinobi의 사용 이력 Consideration과 동일한 Attacker 태그. */
	UPROPERTY(EditAnywhere, Category="Parameter")
	FGameplayTag HistoryTag;
};

USTRUCT(meta=(DisplayName="Lee Boss Attack By Tag", Category="Lee|AI|Action"))
struct GAS_PROJECT_API FLeeBossAttackTask : public FLeeStateTreeTask_ActivateAbility
{
	GENERATED_BODY()
	using FInstanceDataType = FLeeBossAttackData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT(meta=(DisplayName="Lee Boss Can Activate Tag", Category="Lee|AI|Condition"))
struct GAS_PROJECT_API FLeeBossCanActivateAbility : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
	using FInstanceDataType = FLeeBossAttackData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
