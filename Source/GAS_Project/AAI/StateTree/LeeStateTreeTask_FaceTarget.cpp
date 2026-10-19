// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeStateTreeTask_FaceTarget.h"

#include "AIController.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/LeeLogChannels.h"

namespace
{
	// 자신의 정면 벡터와 타겟 방향 사이의 각도가 허용치 이내인지 확인 (ULeeEnemySensingComponent::CanPerceive의 원뿔 판정과 동일 방식)
	bool IsFacingTarget(const AActor* Actor, const AActor* Target, float ToleranceDeg)
	{
		if (!Actor || !Target)
		{
			return false;
		}

		const FVector Forward = Actor->GetActorForwardVector().GetSafeNormal2D();
		const FVector ToTarget = (Target->GetActorLocation() - Actor->GetActorLocation()).GetSafeNormal2D();
		const float DotThreshold = FMath::Cos(FMath::DegreesToRadians(ToleranceDeg));
		return FVector::DotProduct(Forward, ToTarget) >= DotThreshold;
	}
}

// Tick에서 매 프레임 정렬 각도를 확인해야 하므로 계속 호출되어야 한다.
// 완료 판정에는 참여하지 않는다(같은 State의 ActivateAbility Task가 결정) — 결함 A 수정:
// 이 플래그가 없으면 정렬이 끝나는 즉시 State가 완료 처리되어, 발사 전에 ActivateAbility::ExitState가
// 어빌리티를 취소해버린다(26.10.19 분석 문서 참고).
FLeeStateTreeTask_FaceTarget::FLeeStateTreeTask_FaceTarget()
{
	bShouldCallTick = true;
#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
#endif
}

// 상태 진입 — AIController의 Focus를 타겟으로 설정
EStateTreeRunStatus FLeeStateTreeTask_FaceTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.AIController || !Data.TargetActor)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_FaceTarget] AIController 또는 TargetActor가 바인딩되지 않았습니다."));
		return EStateTreeRunStatus::Failed;
	}

	Data.AIController->SetFocus(Data.TargetActor);

	// 이미 정렬돼 있으면 굳이 한 틱을 기다리지 않고 바로 성공 처리
	return IsFacingTarget(Data.Actor, Data.TargetActor, Data.FaceToleranceDeg)
		? EStateTreeRunStatus::Succeeded
		: EStateTreeRunStatus::Running;
}

// 매 틱 — 허용 각도 이내로 정렬되면 완료
EStateTreeRunStatus FLeeStateTreeTask_FaceTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.TargetActor)
	{
		return EStateTreeRunStatus::Failed;
	}

	return IsFacingTarget(Data.Actor, Data.TargetActor, Data.FaceToleranceDeg)
		? EStateTreeRunStatus::Succeeded
		: EStateTreeRunStatus::Running;
}

// 상태 이탈 — Focus 해제
void FLeeStateTreeTask_FaceTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (Data.AIController)
	{
		Data.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
