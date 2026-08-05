// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeStateTreeTask_StrafeAroundTarget.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/LeeLogChannels.h"

namespace
{
	// TargetLocation을 중심으로 AngleDeg 방향, Radius 거리의 지점을 NavMesh에 투영한다.
	// 실패하면 반경을 줄여가며 MaxAttempts번까지 재시도한다.
	bool TryFindStrafeGoal(UWorld* World, const FVector& TargetLocation, float AngleDeg, float Radius, const FVector& Extent, int32 MaxAttempts, FVector& OutGoal)
	{
		UNavigationSystemV1* NavSys = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
		if (!NavSys)
		{
			return false;
		}

		const int32 Attempts = FMath::Max(1, MaxAttempts);
		const float AngleRad = FMath::DegreesToRadians(AngleDeg);
		const FVector Direction(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0.f);

		for (int32 Attempt = 0; Attempt < Attempts; ++Attempt)
		{
			const float AttemptRadius = Radius * (1.f - static_cast<float>(Attempt) / static_cast<float>(Attempts + 1));
			const FVector DesiredPoint = TargetLocation + Direction * FMath::Max(0.f, AttemptRadius);

			FNavLocation NavLoc;
			if (NavSys->ProjectPointToNavigation(DesiredPoint, NavLoc, Extent))
			{
				OutGoal = NavLoc.Location;
				return true;
			}
		}

		return false;
	}
}

// 목표점(각도)은 매 틱, 실제 경로 재요청은 StrafeGoalUpdateInterval 주기로만 하기 위해 Tick이 계속 호출되어야 한다
FLeeStateTreeTask_StrafeAroundTarget::FLeeStateTreeTask_StrafeAroundTarget()
{
	bShouldCallTick = true;
}

// 상태 진입 — 현재 각도/방향/지속시간을 산출하고, 첫 목표 갱신을 다음 Tick에서 즉시 수행하도록 만든다
EStateTreeRunStatus FLeeStateTreeTask_StrafeAroundTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.Actor || !Data.AIController || !Data.TargetActor)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_StrafeAroundTarget] Actor/AIController/TargetActor가 바인딩되지 않았습니다."));
		return EStateTreeRunStatus::Failed;
	}

	const FVector TargetLocation = Data.TargetActor->GetActorLocation();
	const FVector ToActor = (Data.Actor->GetActorLocation() - TargetLocation).GetSafeNormal2D();
	Data.CurrentAngleDeg = FMath::RadiansToDegrees(FMath::Atan2(ToActor.Y, ToActor.X));

	Data.DirectionSign = FMath::RandBool() ? 1.f : -1.f;
	Data.TimeSinceDirectionChange = 0.f;
	Data.NextDirectionChangeInterval = FMath::FRandRange(Data.DirectionChangeInterval * 0.75f, Data.DirectionChangeInterval * 1.25f);

	Data.ElapsedSeconds = 0.f;
	Data.TargetDuration = FMath::FRandRange(Data.MinDuration, Data.MaxDuration);

	// 지금 서 있는 거리에서 시작해 StrafeRadius로 서서히 벌어지게 한다
	Data.CurrentRadius = FVector::Dist2D(Data.Actor->GetActorLocation(), TargetLocation);
	Data.ConsecutiveProjectionFailures = 0;

	// StrafeGoalUpdateInterval만큼 이미 지난 것으로 초기화해 다음 Tick에서 즉시 첫 목표를 요청한다
	Data.TimeSinceLastGoalUpdate = Data.StrafeGoalUpdateInterval;
	Data.MoveRequestId = FAIRequestID::InvalidRequest;

	return EStateTreeRunStatus::Running;
}

// 매 틱 — 지속시간/방향 전환 판정 후 각도를 갱신하고, 주기가 찼을 때만 NavMesh 투영 + 경로 재요청
EStateTreeRunStatus FLeeStateTreeTask_StrafeAroundTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (!Data.Actor || !Data.AIController || !Data.TargetActor)
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.ElapsedSeconds += DeltaTime;
	if (Data.ElapsedSeconds >= Data.TargetDuration)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	Data.TimeSinceDirectionChange += DeltaTime;
	if (Data.TimeSinceDirectionChange >= Data.NextDirectionChangeInterval)
	{
		Data.DirectionSign *= -1.f;
		Data.TimeSinceDirectionChange = 0.f;
		Data.NextDirectionChangeInterval = FMath::FRandRange(Data.DirectionChangeInterval * 0.75f, Data.DirectionChangeInterval * 1.25f);
	}

	// 실제 경로 재요청은 StrafeGoalUpdateInterval 주기로만 (매 틱 요청 금지, 리뷰 §4-2)
	Data.TimeSinceLastGoalUpdate += DeltaTime;
	if (Data.TimeSinceLastGoalUpdate < Data.StrafeGoalUpdateInterval)
	{
		return EStateTreeRunStatus::Running;
	}
	const float UpdateDeltaTime = Data.TimeSinceLastGoalUpdate;
	Data.TimeSinceLastGoalUpdate = 0.f;

	const FVector ActorLocation = Data.Actor->GetActorLocation();
	const FVector TargetLocation = Data.TargetActor->GetActorLocation();

	// 유효 반경: 지금 거리에서 시작해 StrafeRadius로 서서히 회복시킨다.
	// 매번 현재 거리를 그대로 쓰면(FMath::Min) 공격 직후 붙은 거리에 갇혀 간격이 벌어지지 않는다.
	const float CurrentDistance = FVector::Dist2D(ActorLocation, TargetLocation);
	Data.CurrentRadius = FMath::FInterpConstantTo(
		FMath::Min(Data.CurrentRadius > 0.f ? Data.CurrentRadius : CurrentDistance, CurrentDistance),
		Data.StrafeRadius, UpdateDeltaTime, Data.RadiusRecoverySpeed);
	const float EffectiveRadius = FMath::Max(Data.CurrentRadius, 1.f);

	// 목표를 앞에 둘 거리. 도착 판정 반경보다 확실히 멀어야 한다.
	// (짧으면 MoveTo가 AlreadyAtGoal로 이동을 즉시 종료시켜 정지·재출발이 반복된다 — AIController.cpp:710)
	const float SpeedBasedLead = EffectiveRadius * FMath::DegreesToRadians(Data.AngularSpeedDeg) * Data.StrafeGoalUpdateInterval * 2.f;
	const float LeadDistance = FMath::Max(SpeedBasedLead, Data.MinArcAdvanceDistance * 2.f);

	// 각도는 누적하지 않고 매번 실제 위치에서 다시 구한다.
	// 밀리거나 이동 속도가 달라도 목표가 항상 진행 방향 바로 앞에 놓여 부드럽게 이어진다.
	const FVector ToActor = (ActorLocation - TargetLocation).GetSafeNormal2D();
	const float ActualAngleDeg = FMath::RadiansToDegrees(FMath::Atan2(ToActor.Y, ToActor.X));
	const float LeadAngleDeg = FMath::RadiansToDegrees(LeadDistance / EffectiveRadius);
	Data.CurrentAngleDeg = ActualAngleDeg + Data.DirectionSign * LeadAngleDeg;

	FVector Goal;
	bool bFound = TryFindStrafeGoal(Data.Actor->GetWorld(), TargetLocation, Data.CurrentAngleDeg, EffectiveRadius, Data.NavProjectionExtent, Data.MaxProjectionAttempts, Goal);

	if (!bFound && Data.bRetryOppositeOnFail)
	{
		bFound = TryFindStrafeGoal(Data.Actor->GetWorld(), TargetLocation, Data.CurrentAngleDeg + 180.f, EffectiveRadius, Data.NavProjectionExtent, Data.MaxProjectionAttempts, Goal);
	}

	if (!bFound)
	{
		// 한 번 실패로 State를 실패시키면 상위에서 Idle로 튀어 전투가 끊긴다. 연속 실패일 때만 실패 처리한다
		++Data.ConsecutiveProjectionFailures;
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_StrafeAroundTarget] NavMesh 투영 실패 (%d/%d)."),
			Data.ConsecutiveProjectionFailures, Data.MaxConsecutiveProjectionFailures);

		return (Data.ConsecutiveProjectionFailures >= Data.MaxConsecutiveProjectionFailures)
			? EStateTreeRunStatus::Failed
			: EStateTreeRunStatus::Running;
	}
	Data.ConsecutiveProjectionFailures = 0;

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(Goal);
	MoveRequest.SetAcceptanceRadius(Data.MinArcAdvanceDistance);
	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetCanStrafe(true);
	// 캡슐/목표 반경이 더해지면 도착 임계값이 예측 불가해지므로 AcceptanceRadius만 쓰게 고정한다
	MoveRequest.SetReachTestIncludesAgentRadius(false);
	MoveRequest.SetReachTestIncludesGoalRadius(false);

	const FPathFollowingRequestResult Result = Data.AIController->MoveTo(MoveRequest);
	if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		// 여기 들어오면 리드 거리 설정이 잘못된 것 — 이동이 즉시 종료되어 틱틱거린다
		UE_LOG(LogLee, Warning, TEXT("[LeeSTT_StrafeAroundTarget] 목표가 도착 반경 안입니다. Lead=%.0f, Acceptance=%.0f, Radius=%.0f"),
			LeadDistance, Data.MinArcAdvanceDistance, EffectiveRadius);
	}
	Data.MoveRequestId = Result.MoveId;

	return EStateTreeRunStatus::Running;
}

// 상태 이탈 — 이 Task가 발급한 이동 요청만 중단한다 (다른 Task의 이동을 건드리지 않음)
void FLeeStateTreeTask_StrafeAroundTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
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
}
