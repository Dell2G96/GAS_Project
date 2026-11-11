#include "LeeBossAttackNodes.h"
#include "LeeBossBrainComponent.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/AAI/Token/LeeAttackerProfileComponent.h"
#include "GAS_Project/MyTags.h"

bool FLeeBossCanActivateAbility::TestCondition(FStateTreeExecutionContext& Context) const
{
	const auto& Data = Context.GetInstanceData(*this);
	if (!IsValid(Data.TargetActor) || Data.Distance > Data.MaxDistance || !Data.AbilityTag.IsValid()) return false;
	if (auto* Brain = ULeeBossBrainComponent::FindForPawn(Data.Actor); Brain && Brain->IsRecoveringAttackStamina()) return false;
	const auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor);
	if (!ASC) return false;
	// 26.11.11 추가 - 2페이즈 포효 예약 중이면 공격 상태를 고르지 않는다
	if (ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Boss_PhaseChangeQueued)) return false;
	const FGameplayAbilitySpec* Match = nullptr;
	int32 Count = 0;
	for (const auto& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(Data.AbilityTag)) { Match = &Spec; ++Count; }
	}
	return Count == 1 && !Match->IsActive() && Match->Ability->CanActivateAbility(Match->Handle, ASC->AbilityActorInfo.Get());
}

EStateTreeRunStatus FLeeBossAttackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	auto& Data = Context.GetInstanceData(*this);
	if (!Data.AIController || !IsValid(Data.TargetActor)) return EStateTreeRunStatus::Failed;
	Data.AIController->StopMovement();
	Data.AIController->SetFocus(Data.TargetActor);
	Data.bUseTagPollingFallback = false;
	const auto Result = FLeeStateTreeTask_ActivateAbility::EnterState(Context, Transition);
	if (Data.ActivatedSpecHandle.IsValid())
		if (auto* Profile = ULeeAttackerProfileComponent::FindAttackerProfileComponent(Data.Actor)) Profile->RecordUse(Data.HistoryTag);
	return Result;
}

void FLeeBossAttackTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FLeeStateTreeTask_ActivateAbility::ExitState(Context, Transition);
	const auto& Data = Context.GetInstanceData(*this);
	if (Data.AIController) Data.AIController->ClearFocus(EAIFocusPriority::Gameplay);
}
