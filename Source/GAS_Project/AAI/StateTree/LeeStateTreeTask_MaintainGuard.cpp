// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeStateTreeTask_MaintainGuard.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/LeeLogChannels.h"

// 지속시간 판정을 위해 Tick이 계속 호출되어야 한다
FLeeStateTreeTask_MaintainGuard::FLeeStateTreeTask_MaintainGuard()
{
	bShouldCallTick = true;
}

// 상태 진입 — Focus 설정 + Ability_Guard 태그와 정확히 1개 매칭되는 어빌리티 활성화 + 유지 시간 무작위 산출
EStateTreeRunStatus FLeeStateTreeTask_MaintainGuard::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.Actor)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_MaintainGuard] Actor가 바인딩되지 않았습니다."));
		return EStateTreeRunStatus::Failed;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor);
	if (!ASC)
	{
		return EStateTreeRunStatus::Failed;
	}

	FGameplayAbilitySpecHandle MatchedHandle;
	int32 MatchCount = 0;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(MyTags::Souls::Ability_Guard))
		{
			MatchedHandle = Spec.Handle;
			++MatchCount;
		}
	}

	if (MatchCount != 1)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_MaintainGuard] Ability_Guard 태그와 매칭되는 어빌리티가 %d개입니다 (정확히 1개여야 함)."), MatchCount);
		return EStateTreeRunStatus::Failed;
	}

	if (!ASC->TryActivateAbility(MatchedHandle))
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.ActivatedSpecHandle = MatchedHandle;
	Data.ElapsedSeconds = 0.f;
	Data.TargetDuration = FMath::FRandRange(Data.MinDuration, Data.MaxDuration);

	if (Data.AIController && Data.TargetActor)
	{
		Data.AIController->SetFocus(Data.TargetActor);
	}

	return EStateTreeRunStatus::Running;
}

// 매 틱 — Focus를 계속 유지하고, 무작위 산출된 유지 시간이 지나면 완료
EStateTreeRunStatus FLeeStateTreeTask_MaintainGuard::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (Data.AIController && Data.TargetActor)
	{
		Data.AIController->SetFocus(Data.TargetActor);
	}

	Data.ElapsedSeconds += DeltaTime;
	return (Data.ElapsedSeconds >= Data.TargetDuration) ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

// 상태 이탈 — 가드 어빌리티 취소, Focus 해제
void FLeeStateTreeTask_MaintainGuard::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (Data.ActivatedSpecHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor))
		{
			ASC->CancelAbilityHandle(Data.ActivatedSpecHandle);
		}
	}
	Data.ActivatedSpecHandle = FGameplayAbilitySpecHandle();

	if (Data.AIController)
	{
		Data.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
