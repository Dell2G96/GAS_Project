// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeStateTreeTask_MeleeAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/AAI/StateTree/LeeAbilityEndListener.h"

// 종료 감지를 위해 Tick이 계속 호출되어야 한다 (리스너 폴링 + Timeout 판정)
FLeeStateTreeTask_ActivateAbility::FLeeStateTreeTask_ActivateAbility()
{
	bShouldCallTick = true;
}

// 상태 진입 — AbilityTag와 정확히 1개 매칭되는 Spec을 찾아 활성화하고, 종료 감지 리스너를 붙인다
EStateTreeRunStatus FLeeStateTreeTask_ActivateAbility::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.Actor)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_ActivateAbility] Actor가 바인딩되지 않았습니다."));
		return EStateTreeRunStatus::Failed;
	}

	if (!Data.AbilityTag.IsValid())
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_ActivateAbility] AbilityTag가 설정되지 않았습니다."));
		return EStateTreeRunStatus::Failed;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor);
	if (!ASC)
	{
		return EStateTreeRunStatus::Failed;
	}

	// AbilityTag와 정확히 1개 매칭되는 Spec을 찾는다 (0개/2개 이상이면 어빌리티 BP 설정 오류)
	FGameplayAbilitySpecHandle MatchedHandle;
	int32 MatchCount = 0;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(Data.AbilityTag))
		{
			MatchedHandle = Spec.Handle;
			++MatchCount;
		}
	}

	if (MatchCount != 1)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_ActivateAbility] AbilityTag '%s'와 매칭되는 어빌리티가 %d개입니다 (정확히 1개여야 함)."), *Data.AbilityTag.ToString(), MatchCount);
		return EStateTreeRunStatus::Failed;
	}

	if (!ASC->TryActivateAbility(MatchedHandle))
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.ActivatedSpecHandle = MatchedHandle;
	Data.ElapsedSeconds = 0.f;

	if (!Data.EndListener)
	{
		Data.EndListener = NewObject<ULeeAbilityEndListener>(Context.GetOwner());
	}
	Data.EndListener->Bind(ASC, MatchedHandle);

	return EStateTreeRunStatus::Running;
}

// 매 틱 — 1차: 리스너가 감지한 SpecHandle 종료. 2차(fallback): Status_Attack_Attacking 태그 폴링. 마지막: Timeout
EStateTreeRunStatus FLeeStateTreeTask_ActivateAbility::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	Data.ElapsedSeconds += DeltaTime;

	if (Data.EndListener && Data.EndListener->HasEnded())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (Data.bUseTagPollingFallback && Data.ElapsedSeconds > Data.GraceSeconds)
	{
		if (const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor))
		{
			if (!ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Attack_Attacking))
			{
				return EStateTreeRunStatus::Succeeded;
			}
		}
	}

	if (Data.ElapsedSeconds > Data.TimeoutSeconds)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_ActivateAbility] '%s' 어빌리티가 TimeoutSeconds(%.2f초)를 초과했습니다."), *Data.AbilityTag.ToString(), Data.TimeoutSeconds);
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

// 상태 이탈 — 리스너 해제, 아직 활성 중이면(정상 종료가 아니면) 해당 SpecHandle만 취소 (안전장치)
void FLeeStateTreeTask_ActivateAbility::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	const bool bAlreadyEnded = Data.EndListener && Data.EndListener->HasEnded();

	if (Data.EndListener)
	{
		Data.EndListener->Unbind();
	}

	if (!bAlreadyEnded && Data.ActivatedSpecHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor))
		{
			ASC->CancelAbilityHandle(Data.ActivatedSpecHandle);
		}
	}

	Data.ActivatedSpecHandle = FGameplayAbilitySpecHandle();
}
