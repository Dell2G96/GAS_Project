// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "GAS_Project/ACharacter/LeeCharacter.h"
#include "LeeTeamCreationComponent.generated.h"


UCLASS(Blueprintable)
class GAS_PROJECT_API ULeeTeamCreationComponent : public UGameStateComponent
{
	GENERATED_BODY()
public:
	ULeeTeamCreationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//~End of UObject interface

	virtual void BeginPlay() override;

private:
	void OnExperienceLoaded(const class ULeeExperienceDefinition* Experience);

protected:
	UPROPERTY(EditDefaultsOnly, Category = Teams )
	TMap<uint8, TObjectPtr<class ULeeTeamDisplayAsset>> TeamsToCreate;

	UPROPERTY(EditDefaultsOnly, Category=Teams)
	TSubclassOf<class ALeeTeamPublicInfo> PublicTeamInfoClass;

	UPROPERTY(EditDefaultsOnly, Category=Teams)
	TSubclassOf<class ALeeTeamPrivateInfo> PrivateTeamInfoClass;

	// 실제 플레이어(사람)에게 수동으로 부여할 팀 ID (TeamsToCreate에 등록된 ID여야 함)
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	int32 PlayerTeamId = 1;

	// AI 봇 플레이어에게 수동으로 부여할 팀 ID (TeamsToCreate에 등록된 ID여야 함)
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	int32 BotTeamId = 2;

	// PlayerState 없이 AI 컨트롤러로만 움직이는 Enemy(ALeeCharacter)를 위한, 캐릭터 클래스별 세분화된 팀 ID
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	TMap<TSubclassOf<class ALeeCharacter>, int32> EnemyTeamIdOverrides;

	// EnemyTeamIdOverrides에 등록되지 않은 Enemy가 사용할 기본 팀 ID
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	int32 DefaultEnemyTeamId = 2;

public:
	// EnemyClass(또는 그 부모 클래스)에 등록된 팀 ID를 찾아 반환. 없으면 DefaultEnemyTeamId 반환
	UFUNCTION(BlueprintCallable, Category = Teams)
	int32 ResolveEnemyTeamId(TSubclassOf<class ALeeCharacter> EnemyClass) const;


#if WITH_SERVER_CODE
protected:
	virtual void ServerCreateTeams();
	virtual void ServerAssignPlayersToTeams();

	/** Sets the team ID for the given player state. Spectator-only player states will be stripped of any team association. */
	virtual void ServerChooseTeamForPlayer(class ALeePlayerState* PS);

private:
	void OnPlayerInitialized(AGameModeBase* GameMode, AController* NewPlayer);
	void ServerCreateTeam(int32 TeamId, ULeeTeamDisplayAsset* DisplayAsset);

	/** returns the Team ID with the fewest active players, or INDEX_NONE if there are no valid teams */
	int32 GetLeastPopulatedTeamID() const;
#endif
	
};
