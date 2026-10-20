// Fill out your copyright notice in the Description page of Project Settings.
// 26.10.21 신규 작성 — 원거리 공격 쿨타임 동안 EQS로 고른 지점으로 재배치(스트래핑)하는 Task

#include "LeeStateTreeTask_MoveToEQSLocation.h"

#include "AIController.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/LeeLogChannels.h"

// 이동 완료 여부를 매 틱 확인해야 하므로 Tick을 켠다
FLeeStateTreeTask_MoveToEQSLocation::FLeeStateTreeTask_MoveToEQSLocation()
{
	bShouldCallTick = true;
}

// 상태 진입 — EQS를 즉시(동기) 실행해 목표 지점을 구하고, 타겟을 바라본 채 이동을 요청한다
EStateTreeRunStatus FLeeStateTreeTask_MoveToEQSLocation::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedSeconds = 0.f;
	Data.MoveRequestId = FAIRequestID::InvalidRequest;

	if (!Data.Actor || !Data.AIController || !Data.TargetActor || !Data.QueryTemplate)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_MoveToEQSLocation] Actor/AIController/TargetActor/QueryTemplate가 비어 있습니다."));
		return EStateTreeRunStatus::Failed;
	}

	UEnvQueryManager* EQSManager = UEnvQueryManager::GetCurrent(Data.Actor);
	if (!EQSManager)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 상태에 들어올 때 한 번만 실행하므로 비동기 대신 즉시 실행을 쓴다 (콜백/수명 관리가 필요 없어 단순하다)
	const FEnvQueryRequest Request(Data.QueryTemplate, Data.Actor);
	const TSharedPtr<FEnvQueryResult> Result = EQSManager->RunInstantQuery(Request, EEnvQueryRunMode::SingleResult);
	if (!Result.IsValid() || !Result->IsSuccessful() || Result->Items.Num() == 0)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_MoveToEQSLocation] %s: EQS(%s) 결과가 없습니다."),
			*GetNameSafe(Data.Actor), *GetNameSafe(Data.QueryTemplate));
		return EStateTreeRunStatus::Failed;
	}

	const FVector Goal = Result->GetItemAsLocation(0);

	// Focus를 타겟으로 고정해야 몸은 타겟을 보면서 옆/뒤로 움직인다 (StrafeAroundTarget과 같은 이유)
	Data.AIController->SetFocus(Data.TargetActor);

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(Goal);
	MoveRequest.SetAcceptanceRadius(Data.AcceptanceRadius);
	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetCanStrafe(true);
	MoveRequest.SetReachTestIncludesAgentRadius(false);
	MoveRequest.SetReachTestIncludesGoalRadius(false);

	const FPathFollowingRequestResult MoveResult = Data.AIController->MoveTo(MoveRequest);
	if (MoveResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EStateTreeRunStatus::Succeeded;
	}
	if (MoveResult.Code == EPathFollowingRequestResult::Failed)
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.MoveRequestId = MoveResult.MoveId;
	return EStateTreeRunStatus::Running;
}

// 매 틱 — 이동이 끝났거나(Idle) 최대 시간이 지나면 Succeeded
EStateTreeRunStatus FLeeStateTreeTask_MoveToEQSLocation::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.ElapsedSeconds += DeltaTime;
	if (Data.ElapsedSeconds >= Data.MaxMoveDuration)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 우리 이동 요청이 끝났으면 PathFollowing이 Idle로 돌아간다
	const UPathFollowingComponent* PathFollowingComp = Data.AIController->GetPathFollowingComponent();
	if (!PathFollowingComp || PathFollowingComp->GetStatus() == EPathFollowingStatus::Idle)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

// 상태 이탈 — 이 Task가 발급한 이동 요청만 중단하고 Focus를 푼다
void FLeeStateTreeTask_MoveToEQSLocation::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (Data.AIController && Data.MoveRequestId.IsValid())
	{
		if (UPathFollowingComponent* PathFollowingComp = Data.AIController->GetPathFollowingComponent())
		{
			PathFollowingComp->AbortMove(*Data.AIController, FPathFollowingResultFlags::OwnerFinished, Data.MoveRequestId);
		}
	}
	Data.MoveRequestId = FAIRequestID::InvalidRequest;

	if (Data.AIController)
	{
		Data.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
