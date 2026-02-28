// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LeeGameModeBase.generated.h"

class ULeePawnData;

UCLASS()
class GAS_PROJECT_API ALeeGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALeeGameModeBase();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() final;

	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) final;
	
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) final;

	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) final;

	void HandleMatchAssignmentIfNotExpectingOne();
	void OnMatchAssignmentGiven(const FPrimaryAssetId& ExperienceId);
	bool IsExperienceLoaded() const;
	void OnExperienceLoaded(const class ULeeExperienceDefinition* CurrentExperience);
	const  ULeePawnData* GetPawnDataForController(const AController* InController) const;
};
