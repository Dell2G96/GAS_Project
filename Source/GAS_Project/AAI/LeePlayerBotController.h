// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "ModularPlayerController.h"
#include "GAS_Project/ATeam/LeeTeamAgentInterface.h"
#include "LeePlayerBotController.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class GAS_PROJECT_API ALeePlayerBotController : public AModularAIController, public ILeeTeamAgentInterface
{
	GENERATED_BODY()

public:
	ALeePlayerBotController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ILeeTeamAgentInterface 
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnLeeTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	// ~ILeeTeamAgentInterface

	void ServerRestartController();

	UFUNCTION(BlueprintCallable, Category= "Lee AI Player Controller")
	void UpdateTeamAttitude(UAIPerceptionComponent* AIPerception);

	virtual void OnUnPossess() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=BotIdentifier)
	int32 BotIdentifier;

private:
	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

protected:
	virtual void OnPlayerStateChanged();

private:
	void BroadcastOnPlayerStateChanged();

protected:
	// AController Interface
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	// ~AController

private:
	UPROPERTY()
	FOnLeeTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;

	
};
