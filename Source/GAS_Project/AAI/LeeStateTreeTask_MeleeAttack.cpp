// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeStateTreeTask_MeleeAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/MyTags.h"

FLeeStateTreeTask_MeleeAttack::FLeeStateTreeTask_MeleeAttack()
{
	// 공격 종료(Attacking 태그 소멸)를 감지하려면 Tick이 계속 호출되어야 한다
	bShouldCallTick = true;
}

// 상태 진입 — 이미 공격 중이면 실패(AI 재평가), 아니면 지정된 태그의 공격 어빌리티 활성화
EStateTreeRunStatus FLeeStateTreeTask_MeleeAttack::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.Actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeSTT_MeleeAttack] Actor가 바인딩되지 않았습니다."));
		return EStateTreeRunStatus::Failed;
	}

	if (!Data.AttackAbilityTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeSTT_MeleeAttack] AttackAbilityTag가 설정되지 않았습니다."));
		return EStateTreeRunStatus::Failed;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor);
	if (!ASC)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 이미 다른 근접 공격이 진행 중이면 새 요청은 실패 처리하고 AI가 다시 판단하게 한다
	if (ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Attack_Melee))
	{
		return EStateTreeRunStatus::Failed;
	}

	if (!ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(Data.AttackAbilityTag)))
	{
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

// 매 틱 — 공격 어빌리티의 종료를 Status_Attack_Attacking 태그 소멸로 판단 (콤보 등 내부 진행은 알 필요 없음)
EStateTreeRunStatus FLeeStateTreeTask_MeleeAttack::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor);
	if (!ASC)
	{
		return EStateTreeRunStatus::Failed;
	}

	return ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Attack_Attacking)
		? EStateTreeRunStatus::Running
		: EStateTreeRunStatus::Succeeded;
}

// 상태 이탈 — 정상 종료가 아니라 외부 전이로 강제 이탈했고 공격이 아직 진행 중이면 취소 (안전장치)
void FLeeStateTreeTask_MeleeAttack::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor);
	if (!ASC || !Data.AttackAbilityTag.IsValid())
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Attack_Attacking))
	{
		const FGameplayTagContainer CancelTags(Data.AttackAbilityTag);
		ASC->CancelAbilities(&CancelTags);
	}
}
