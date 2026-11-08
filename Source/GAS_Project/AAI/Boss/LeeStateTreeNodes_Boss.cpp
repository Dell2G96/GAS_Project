// 26.11.05 신규 - Stone 보스 AI (Behavior Tree + StateTree 혼용)

#include "LeeStateTreeNodes_Boss.h"
#include "LeeBossBrainComponent.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"

// 회복 중이거나 공격 중이면 방어로 전환하지 않는다.
bool FLeeBossCanReactWithGuard::TestCondition(FStateTreeExecutionContext& Context) const
{
	const auto& Data = Context.GetInstanceData(*this);
	auto* Brain = ULeeBossBrainComponent::FindForPawn(Data.Actor);
	// 26.11.08 수정 - 공격 감지는 평가기 bShouldGuardNow로 받는다 (보스 전용: 공격 스태미나 회복 중이면 가드 안 함)
	if (!Data.bIsCloseTier || !Data.bIsTargetInFront || !Data.bShouldGuardNow || !Brain
		|| Brain->IsRecoveringAttackStamina())
	{
		return false;
	}

	const auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor);
	return ASC && !ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Attack_Attacking)
		&& ASC->GetNumericAttribute(ULeeSoulsStatSet::GetStaminaAttribute()) > 0.f;
}

bool FLeeStateTreeCondition_DistanceInRange::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	return Data.Distance >= Data.MinDistance && Data.Distance <= Data.MaxDistance;
}

// 공격 후 대기를 시작하고, 비용 고갈 상태라면 스태미나가 회복될 때까지 유지한다.
EStateTreeRunStatus FLeeStateTreeTask_BossRecovery::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	const float MinD = FMath::Min(Data.MinDuration, Data.MaxDuration);
	const float MaxD = FMath::Max(Data.MinDuration, Data.MaxDuration);
	Data.RemainingTime = FMath::FRandRange(MinD, MaxD);

	if (Data.bKeepFocusOnTarget && Data.AIController && Data.TargetActor)
	{
		Data.AIController->SetFocus(Data.TargetActor);
	}

	return Tick(Context, 0.f);
}

// 기존 후딜과 자기 공격 비용에 대한 스태미나 회복이 모두 끝났는지 확인한다.
EStateTreeRunStatus FLeeStateTreeTask_BossRecovery::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.RemainingTime -= DeltaTime;
	if (auto* Brain = Data.AIController ? Data.AIController->FindComponentByClass<ULeeBossBrainComponent>() : nullptr;
		Brain && Brain->IsRecoveringAttackStamina())
	{
		return EStateTreeRunStatus::Running;
	}
	return Data.RemainingTime <= 0.f ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FLeeStateTreeTask_BossRecovery::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (Data.bKeepFocusOnTarget && Data.AIController)
	{
		Data.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
