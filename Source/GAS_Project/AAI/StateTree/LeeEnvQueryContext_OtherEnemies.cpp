// Fill out your copyright notice in the Description page of Project Settings.
// 26.11.25 신규 작성 — 다른 Enemy가 "가는 중인 목적지"를 EQS에 넘겨 같은 지점으로 몰리지 않게 하는 컨텍스트

#include "LeeEnvQueryContext_OtherEnemies.h"

#include "AIController.h"
#include "EngineUtils.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationData.h"
#include "GAS_Project/AAI/LeePlayerBotController.h"
#include "GAS_Project/ACharacter/LeeHealthComponent.h"

namespace
{
	// 이 Enemy가 "차지할 위치"를 구한다.
	// 위치로 이동 중이면 경로 끝(목적지), 그 외(서 있음 / 액터를 쫓는 중)는 현재 위치.
	// 액터를 쫓는 중일 때 목적지를 쓰지 않는 이유: 목적지가 플레이어 위치라서 플레이어 주변 전체가 막혀 버린다.
	FVector GetClaimedLocation(const APawn* Pawn, const AAIController* AIController)
	{
		const UPathFollowingComponent* PathFollowing = AIController->GetPathFollowingComponent();
		if (PathFollowing
			&& PathFollowing->GetStatus() == EPathFollowingStatus::Moving
			&& PathFollowing->GetMoveGoal() == nullptr
			&& PathFollowing->GetPath().IsValid())
		{
			// 경로 점이 비어 있으면 InvalidLocation이 나오므로 그때는 현재 위치를 쓴다
			const FVector PathEnd = PathFollowing->GetPath()->GetEndLocation();
			if (FNavigationSystem::IsValidLocation(PathEnd))
			{
				return PathEnd;
			}
		}

		return Pawn->GetActorLocation();
	}
}

// 자기 자신을 뺀, 살아 있는 다른 Enemy들의 목적지 또는 현재 위치를 EQS 컨텍스트로 넘긴다
void ULeeEnvQueryContext_OtherEnemies::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	const AActor* Querier = Cast<AActor>(QueryInstance.Owner.Get());

	TArray<FVector> Locations;
	GatherOtherEnemyLocations(Querier, Locations);

	// 다른 Enemy가 없으면 아무것도 넣지 않는다 (거리 테스트가 이 컨텍스트를 건너뛴다)
	if (Locations.Num() > 0)
	{
		UEnvQueryItemType_Point::SetContextHelper(ContextData, Locations);
	}
}

// Self를 뺀 살아 있는 다른 Enemy(플레이어 봇 제외)들의 목적지 또는 현재 위치를 모은다
void ULeeEnvQueryContext_OtherEnemies::GatherOtherEnemyLocations(const AActor* Self, TArray<FVector>& OutLocations)
{
	OutLocations.Reset();

	UWorld* World = Self ? Self->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	for (TActorIterator<APawn> It(World); It; ++It)
	{
		const APawn* Pawn = *It;
		if (Pawn == Self)
		{
			continue;
		}

		// Enemy만: AI가 조종하는 폰 중 플레이어 봇은 뺀다
		const AAIController* AIController = Cast<AAIController>(Pawn->GetController());
		if (!AIController || AIController->IsA<ALeePlayerBotController>())
		{
			continue;
		}

		// 죽었거나 죽는 중인 Enemy는 뺀다
		const ULeeHealthComponent* Health = ULeeHealthComponent::FindHealthComponent(Pawn);
		if (Health && Health->IsDeadOrDying())
		{
			continue;
		}

		OutLocations.Add(GetClaimedLocation(Pawn, AIController));
	}
}
