// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeTeamCreationComponent.h"

#include "LeeTeamPrivateInfo.h"
#include "LeeTeamPublicInfo.h"
#include "GAS_Project/APlayer/LeePlayerSpawningManagerComponent.h"
#include "GAS_Project/APlayer/LeePlayerState.h"
#include "GAS_Project/GameModes/LeeExperienceManagerComponent.h"
#include "GAS_Project/GameModes/LeeGameModeBase.h"


#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif



ULeeTeamCreationComponent::ULeeTeamCreationComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PublicTeamInfoClass = ALeeTeamPublicInfo::StaticClass();
	PrivateTeamInfoClass = ALeeTeamPrivateInfo::StaticClass();
}

#if WITH_EDITOR
EDataValidationResult ULeeTeamCreationComponent::IsDataValid(class FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	//@TODO: TEAMS: Validate that all display assets have the same properties set!

	return Result;}

void ULeeTeamCreationComponent::BeginPlay()
{
	Super::BeginPlay();

	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	ULeeExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
	check(ExperienceComponent);
	
	ExperienceComponent->CallOrRegister_OnExperienceLoaded_HighPriority(FOnLeeExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));

}
#endif

int32 ULeeTeamCreationComponent::ResolveEnemyTeamId(TSubclassOf<ALeeCharacter> EnemyClass) const
{
	// EnemyClass부터 부모 클래스로 거슬러 올라가며 EnemyTeamIdOverrides에 등록된 항목을 찾는다
	for (UClass* TestClass = EnemyClass; TestClass; TestClass = TestClass->GetSuperClass())
	{
		if (const int32* FoundTeamId = EnemyTeamIdOverrides.Find(TSubclassOf<ALeeCharacter>(TestClass)))
		{
			return *FoundTeamId;
		}
	}

	return DefaultEnemyTeamId;
}

void ULeeTeamCreationComponent::OnExperienceLoaded(const class ULeeExperienceDefinition* Experience)
{
#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		ServerCreateTeams();
		ServerAssignPlayersToTeams();
	}
#endif
}


#if WITH_SERVER_CODE
void ULeeTeamCreationComponent::ServerCreateTeams()
{
	for (const auto& KVP : TeamsToCreate)
	{
		const int32 TeamId = KVP.Key;
		ServerCreateTeam(TeamId, KVP.Value);
	}
}

void ULeeTeamCreationComponent::ServerAssignPlayersToTeams()
{
	// Assign players that already exist to teams
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (ALeePlayerState* LeePS = Cast<ALeePlayerState>(PS))
		{
			ServerChooseTeamForPlayer(LeePS);
		}
	}

	// ToCheck ALeeGameMode(원본) -> ALeeGameModeBase로 수정
	// Listen for new players logging in
	ALeeGameModeBase* GameMode = Cast<ALeeGameModeBase>(GameState->AuthorityGameMode);
	check(GameMode);
	
	GameMode->OnGameModePlayerInitialized.AddUObject(this, &ThisClass::OnPlayerInitialized);
}

void ULeeTeamCreationComponent::ServerChooseTeamForPlayer(class ALeePlayerState* PS)
{
	// 관전자는 어떤 팀에도 속하지 않는다
	if (PS->IsOnlyASpectator())
	{
		PS->SetGenericTeamId(FGenericTeamId::NoTeam);
	}
	// 그 외에는 자동 계산(GetLeastPopulatedTeamID) 대신, 에디터에서 지정한 PlayerTeamId / BotTeamId를 그대로 사용한다
	else
	{
		if (PS->IsABot())
		{
			PS->SetGenericTeamId(IntegerToGenericTeamId(BotTeamId));
		}
		else
		{
			PS->SetGenericTeamId(IntegerToGenericTeamId(PlayerTeamId));
		}
	}

	// [임시 디버그] 실제로 어떤 TeamID가 부여됐는지 확인
	UE_LOG(LogTemp, Warning, TEXT("[Team] ServerChooseTeamForPlayer: %s → TeamId=%d"), *GetNameSafe(PS), PS->GetTeamId());
}

void ULeeTeamCreationComponent::OnPlayerInitialized(AGameModeBase* GameMode, AController* NewPlayer)
{
	check(NewPlayer);
	check(NewPlayer->PlayerState);
	if (ALeePlayerState* LeePS = Cast<ALeePlayerState>(NewPlayer->PlayerState))
	{
		ServerChooseTeamForPlayer(LeePS);
	}
}

void ULeeTeamCreationComponent::ServerCreateTeam(int32 TeamId, ULeeTeamDisplayAsset* DisplayAsset)
{
	check(HasAuthority());

	//@TODO: ensure the team doesn't already exist

	UWorld* World = GetWorld();
	check(World);

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ALeeTeamPublicInfo* NewTeamPublicInfo = World->SpawnActor<ALeeTeamPublicInfo>(PublicTeamInfoClass, SpawnInfo);
	checkf(NewTeamPublicInfo != nullptr, TEXT("Failed to create public team actor from class %s"), *GetPathNameSafe(*PublicTeamInfoClass));
	NewTeamPublicInfo->SetTeamId(TeamId);
	NewTeamPublicInfo->SetTeamDisplayAsset(DisplayAsset);

	ALeeTeamPrivateInfo* NewTeamPrivateInfo = World->SpawnActor<ALeeTeamPrivateInfo>(PrivateTeamInfoClass, SpawnInfo);
	checkf(NewTeamPrivateInfo != nullptr, TEXT("Failed to create private team actor from class %s"), *GetPathNameSafe(*PrivateTeamInfoClass));
	NewTeamPrivateInfo->SetTeamId(TeamId);
}

int32 ULeeTeamCreationComponent::GetLeastPopulatedTeamID() const
{
	const int32 NumTeams = TeamsToCreate.Num();
	if (NumTeams > 0)
	{
		TMap<int32, uint32> TeamMemberCounts;
		TeamMemberCounts.Reserve(NumTeams);

		for (const auto& KVP : TeamsToCreate)
		{
			const int32 TeamId = KVP.Key;
			TeamMemberCounts.Add(TeamId, 0);
		}

		AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (ALeePlayerState* LeePS = Cast<ALeePlayerState>(PS))
			{
				const int32 PlayerTeamID = LeePS->GetTeamId();

				if ((PlayerTeamID != INDEX_NONE) && !LeePS->IsInactive())	// do not count unassigned or disconnected players
				{
					check(TeamMemberCounts.Contains(PlayerTeamID))
					TeamMemberCounts[PlayerTeamID] += 1;
				}
			}
		}

		// sort by lowest team population, then by team ID
		int32 BestTeamId = INDEX_NONE;
		uint32 BestPlayerCount = TNumericLimits<uint32>::Max();
		for (const auto& KVP : TeamMemberCounts)
		{
			const int32 TestTeamId = KVP.Key;
			const uint32 TestTeamPlayerCount = KVP.Value;

			if (TestTeamPlayerCount < BestPlayerCount)
			{
				BestTeamId = TestTeamId;
				BestPlayerCount = TestTeamPlayerCount;
			}
			else if (TestTeamPlayerCount == BestPlayerCount)
			{
				if ((TestTeamId < BestTeamId) || (BestTeamId == INDEX_NONE))
				{
					BestTeamId = TestTeamId;
					BestPlayerCount = TestTeamPlayerCount;
				}
			}
		}

		return BestTeamId;
	}

	return INDEX_NONE;
}
#endif	// WITH_SERVER_CODE

