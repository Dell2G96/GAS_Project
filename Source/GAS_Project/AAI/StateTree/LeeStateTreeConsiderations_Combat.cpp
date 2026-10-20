// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeStateTreeConsiderations_Combat.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/AAI/Token/LeeAttackerProfileComponent.h"
#include "GAS_Project/LeeLogChannels.h"

// 마지막 사용 후 경과 시간을 InputRange로 정규화한 뒤 ResponseCurve로 평가 (한 번도 안 썼으면 최댓값 취급)
float FLeeConsideration_TimeSinceLastUse::GetScore(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	const ULeeAttackerProfileComponent* Profile = Data.RequesterPawn
		? ULeeAttackerProfileComponent::FindAttackerProfileComponent(Data.RequesterPawn)
		: nullptr;

	// 점수 0인 State는 Utility 선택에서 아예 제외되므로, 태그 누락은 조용히 넘기지 않고 알린다(26.10.20).
	// 매 평가마다 찍히면 로그가 넘치므로 실행 중 한 번만 출력한다.
	if (!Data.AttackTag.IsValid())
	{
		static bool bWarnedEmptyTag = false;
		if (!bWarnedEmptyTag)
		{
			bWarnedEmptyTag = true;
			UE_LOG(LogLee, Warning, TEXT("[LeeConsideration_TimeSinceLastUse] AttackTag가 비어 있어 점수가 0입니다. 이 State는 Utility 선택에서 제외됩니다. (Owner: %s)"),
				*GetNameSafe(Context.GetOwner()));
		}
		return 0.f;
	}

	if (!Profile)
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
