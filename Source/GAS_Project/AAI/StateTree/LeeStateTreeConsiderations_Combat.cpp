// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeStateTreeConsiderations_Combat.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/AAI/Token/LeeAttackerProfileComponent.h"

// 마지막 사용 후 경과 시간을 InputRange로 정규화한 뒤 ResponseCurve로 평가 (한 번도 안 썼으면 최댓값 취급)
float FLeeConsideration_TimeSinceLastUse::GetScore(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	const ULeeAttackerProfileComponent* Profile = Data.RequesterPawn
		? ULeeAttackerProfileComponent::FindAttackerProfileComponent(Data.RequesterPawn)
		: nullptr;

	if (!Profile || !Data.AttackTag.IsValid())
	{
		return 0.f;
	}

	const double TimeSinceLastUse = Profile->GetTimeSinceLastUse(Data.AttackTag);
	const float ClampedTime = FMath::IsFinite(static_cast<float>(TimeSinceLastUse))
		? static_cast<float>(TimeSinceLastUse)
		: Data.Interval.Max;

	const float NormalizedInput = Data.Interval.Size() > KINDA_SMALL_NUMBER
		? FMath::GetRangePct(Data.Interval.Min, Data.Interval.Max, ClampedTime)
		: 0.f;

	return ResponseCurve.Evaluate(FMath::Clamp(NormalizedInput, 0.f, 1.f));
}

// 타겟 ASC가 TargetTag를 보유하고 있는지에 따라 두 점수 중 하나를 반환
float FLeeConsideration_TargetHasTag::GetScore(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.TargetActor || !Data.TargetTag.IsValid())
	{
		return Data.ScoreWhenAbsent;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.TargetActor);
	const bool bHasTag = ASC && ASC->HasMatchingGameplayTag(Data.TargetTag);

	return bHasTag ? Data.ScoreWhenPresent : Data.ScoreWhenAbsent;
}
