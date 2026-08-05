// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeStateTreeTask_SetMovementGait.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/LeeLogChannels.h"

// 완료 판정에 참여하지 않고(같은 State의 MoveTo 등이 완료를 결정), Tick 없이 적용/복원만 담당한다
FLeeStateTreeTask_SetMovementGait::FLeeStateTreeTask_SetMovementGait()
{
	bShouldCallTick = false;
#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
#endif
}

// 상태 진입 — 현재 MaxWalkSpeed를 캐시한 뒤 Gait에 맞는 속도로 대입
EStateTreeRunStatus FLeeStateTreeTask_SetMovementGait::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	ACharacter* Character = Cast<ACharacter>(Data.Actor);
	UCharacterMovementComponent* MoveComp = Character ? Character->GetCharacterMovement() : nullptr;
	if (!MoveComp)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_SetMovementGait] Actor가 ACharacter가 아니거나 CharacterMovementComponent가 없습니다."));
		return EStateTreeRunStatus::Running;
	}

	Data.CachedMaxWalkSpeed = MoveComp->MaxWalkSpeed;
	Data.bHasCachedSpeed = true;

	MoveComp->MaxWalkSpeed = (Data.Gait == ELeeMovementGait::Run) ? Data.RunSpeed : Data.WalkSpeed;

	return EStateTreeRunStatus::Running;
}

// 상태 이탈 — 캐시해둔 원래 MaxWalkSpeed로 복원
void FLeeStateTreeTask_SetMovementGait::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.bHasCachedSpeed)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(Data.Actor);
	if (UCharacterMovementComponent* MoveComp = Character ? Character->GetCharacterMovement() : nullptr)
	{
		MoveComp->MaxWalkSpeed = Data.CachedMaxWalkSpeed;
	}

	Data.bHasCachedSpeed = false;
}
