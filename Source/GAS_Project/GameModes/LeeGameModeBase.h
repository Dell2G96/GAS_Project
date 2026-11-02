// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"
#include "GameFramework/GameModeBase.h"
#include "LeeGameModeBase.generated.h"

class ULeePawnData;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLyraGameModePlayerInitialized, AGameModeBase* /*GameMode*/, AController* /*NewPlayer*/);


UCLASS(Config = Game, Meta = (ShortTooltip = "The base game mode class used by this project."))
class GAS_PROJECT_API ALeeGameModeBase : public AModularGameModeBase
{
	GENERATED_BODY()

public:
	ALeeGameModeBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	

	UFUNCTION(BlueprintCallable, Category = "Lyra|Pawn")
	 const class ULeePawnData* GetPawnDataForController(const AController* InController) const;


	//~AGameModeBase interface
	 virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	 virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	 virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
	 virtual bool ShouldSpawnAtStartSpot(AController* Player) override;
	 virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	 virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	 virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;
	 virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;
	 virtual void InitGameState() override;
	 virtual bool UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage) override;
	 virtual void GenericPlayerInitialization(AController* NewPlayer) override;
	 virtual void FailedToRestartPlayer(AController* NewPlayer) override;
	//~End of AGameModeBase interface


	// Restart (respawn) the specified player or bot next frame
	// - If bForceReset is true, the controller will be reset this frame (abandoning the currently possessed pawn, if any)
	UFUNCTION(BlueprintCallable)
	 void RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset = false);

	// Agnostic version of PlayerCanRestart that can be used for both player bots and players
	 virtual bool ControllerCanRestart(AController* Controller);

	// Delegate called on player initialization, described above 
	FOnLyraGameModePlayerInitialized OnGameModePlayerInitialized;

protected:	
	 void OnExperienceLoaded(const class ULeeExperienceDefinition* CurrentExperience);
	 bool IsExperienceLoaded() const;

	 void OnMatchAssignmentGiven(FPrimaryAssetId ExperienceId, const FString& ExperienceIdSource);

	 void HandleMatchAssignmentIfNotExpectingOne();

	// 맵의 Experience ID 조회를 처음 시도한 시각 (별도 프로세스에서 에셋 스캔 반영을 기다릴 때 사용)
	double ExperienceResolveStartTime = -1.0;

	// Experience ID 조회 재시도 최대 대기 시간(초)
	static constexpr double ExperienceResolveTimeout = 30.0;

	// Primary Asset 강제 재스캔을 이미 했는지
	bool bForcedExperienceRescan = false;

	// 별도 프로세스(-game)에서 누락된 Primary Asset 스캔을 보완 (등록된 모든 타입을 동기 스캔)
	void RescanAllPrimaryAssetTypes();

	 bool TryDedicatedServerLogin();
	// void HostDedicatedServerMatch(ECommonSessionOnlineMode OnlineMode);

	// UFUNCTION()
	//  void OnUserInitializedForDedicatedServer(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext);
	
	
	// virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	// virtual void InitGameState() final;
	//
	// virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) final;
	//
	// virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) final;
	//
	// virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) final;
	//
	// void HandleMatchAssignmentIfNotExpectingOne();
	void OnMatchAssignmentGiven(const FPrimaryAssetId& ExperienceId);
	// bool IsExperienceLoaded() const;
	// void OnExperienceLoaded(const class ULeeExperienceDefinition* CurrentExperience);
	// const  ULeePawnData* GetPawnDataForController(const AController* InController) const;
};
