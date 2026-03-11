// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "LeeGameSession.generated.h"

UCLASS(Config = Game)
class GAS_PROJECT_API ALeeGameSession : public AGameSession
{
	GENERATED_BODY()

public:
	ALeeGameSession(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	/** Override to disable the default behavior */
	virtual bool ProcessAutoLogin() override;

	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

};
