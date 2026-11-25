// Fill out your copyright notice in the Description page of Project Settings.
// 26.10.21 신규 작성 — 원거리 공격 쿨타임 동안 EQS로 고른 지점으로 재배치(스트래핑)하는 Task
// 26.11.25 수정 — 주기적 재계산(RequeryInterval)·진행 방향 보기·EQS 실패 시 타겟 직행 옵션 추가 (빈 지점으로 접근)

#include "LeeStateTreeTask_MoveToEQSLocation.h"

#include "AIController.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/LeeLogChannels.h"

namespace
{
	// EQS를 즉시(동기) 실행해 1등 지점을 구한다. 결과가 없으면 false
	bool RunQueryForGoal(const FLeeMoveToEQSLocationInstanceData& Data, FVector& OutGoal)
	{
		UEnvQueryManager* EQSManager = UEnvQueryManager::GetCurrent(Data.Actor);
		if (!EQSManager)
		{
			return false;
		}

		// 비동기 대신 즉시 실행을 쓴다 (콜백/수명 관리가 필요 없어 단순하다)
		const FEnvQueryRequest Request(Data.QueryTemplate, Data.Actor);
		const TSharedPtr<FEnvQueryResult> Result = EQSManager->RunInstantQuery(Request, EEnvQueryRunMode::SingleResult);
		if (!Result.IsValid() || !Result->IsSuccessful() || Result->Items.Num() == 0)
		{
			return false;
		}

		OutGoal = Result->GetItemAsLocation(0);
		return true;
	}

	// EQS로 고른 지점(없으면 설정에 따라 타겟)으로 이동을 요청하고 결과를 StateTree 상태로 돌려준다
	EStateTreeRunStatus RequestMove(FLeeMoveToEQSLocationInstanceData& Data)
	{
		FAIMoveRequest MoveRequest;
		MoveRequest.SetAcceptanceRadius(Data.AcceptanceRadius);
		MoveRequest.SetUsePathfinding(true);
		MoveRequest.SetCanStrafe(Data.bFaceTargetWhileMoving);
		MoveRequest.SetReachTestIncludesAgentRadius(false);
		MoveRequest.SetReachTestIncludesGoalRadius(false);

		FVector Goal;
		if (RunQueryForGoal(Data, Goal))
		{
			MoveRequest.SetGoalLocation(Goal);
		}
		else if (Data.bMoveToTargetOnQueryFail)
		{
			// 빈 지점이 하나도 없으면 예전처럼 타겟에게 바로 다가간다
			MoveRequest.SetGoalActor(Data.TargetActor);
		}
		else
		{
			UE_LOG(LogLee, Warning, TEXT("[LeeSTT_MoveToEQSLocation] %s: EQS(%s) 결과가 없습니다."),
				*GetNameSafe(Data.Actor), *GetNameSafe(Data.QueryTemplate));
			return EStateTreeRunStatus::Failed;
		}

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
}

// 이동 완료 여부를 매 틱 확인해야 하므로 Tick을 켠다
FLeeStateTreeTask_MoveToEQSLocation::FLeeStateTreeTask_MoveToEQSLocation()
{
	bShouldCallTick = true;
}

// 상태 진입 — EQS로 목표 지점을 구해 이동을 요청한다 (설정에 따라 타겟을 바라본 채 이동)
EStateTreeRunStatus FLeeStateTreeTask_MoveToEQSLocation::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedSeconds = 0.f;
	Data.TimeSinceQuery = 0.f;
	Data.MoveRequestId = FAIRequestID::InvalidRequest;

	if (!Data.Actor || !Data.AIController || !Data.TargetActor || !Data.QueryTemplate)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_MoveToEQSLocation] Actor/AIController/TargetActor/QueryTemplate가 비어 있습니다."));
		return EStateTreeRunStatus::Failed;
	}

	// Focus를 타겟으로 고정해야 몸은 타겟을 보면서 옆/뒤로 움직인다 (StrafeAroundTarget과 같은 이유)
	if (Data.bFaceTargetWhileMoving)
	{
		Data.AIController->SetFocus(Data.TargetActor);
	}

	return RequestMove(Data);
}

// 매 틱 — 재계산 주기가 되면 목표를 다시 구하고, 이동이 끝났거나(Idle) 최대 시간이 지나면 Succeeded
EStateTreeRunStatus FLeeStateTreeTask_MoveToEQSLocation::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.AIController || !Data.Actor || !Data.TargetActor)
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.ElapsedSeconds += DeltaTime;
	if (Data.ElapsedSeconds >= Data.MaxMoveDuration)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 26.11.25 추가 - 타겟이 움직이면 빈 지점도 바뀌므로 주기적으로 다시 고른다
	if (Data.RequeryInterval > 0.f)
	{
		Data.TimeSinceQuery += DeltaTime;
		if (Data.TimeSinceQuery >= Data.RequeryInterval)
		{
			Data.TimeSinceQuery = 0.f;
			const EStateTreeRunStatus Status = RequestMove(Data);
			if (Status != EStateTreeRunStatus::Running)
			{
				return Status;
			}
		}
	}

	// 우리 이동 요청이 끝났으면 PathFollowing이 Idle로 돌아간다
	const UPathFollowingComponent* PathFollowingComp = Data.AIController->GetPathFollowingComponent();
	if (!PathFollowingComp || PathFollowingComp->GetStatus() == EPathFollowingStatus::Idle)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

// 상태 이탈 — 이 Task가 발급한 이동 요청만 중단하고, 직접 건 Focus만 푼다
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

	if (Data.AIController && Data.bFaceTargetWhileMoving)
	{
		Data.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
