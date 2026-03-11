// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameSession.h"

ALeeGameSession::ALeeGameSession(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool ALeeGameSession::ProcessAutoLogin()
{
	// This is actually handled in LyraGameMode::TryDedicatedServerLogin
	return true;

}

void ALeeGameSession::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
}

void ALeeGameSession::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
}
