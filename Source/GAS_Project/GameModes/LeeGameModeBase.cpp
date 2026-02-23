// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameModeBase.h"

ALeeGameModeBase::ALeeGameModeBase()
{
	// GameStateClass = ALeeGameState::StaticClass();
	// PlayerControllerClass = 
}

void ALeeGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);


	// 아직 게임인스턴스를 통해 초기화 작업이 진행되므로
	// 현 프레임에서는 Lyra의 Experience 를 처리 할 수 없음
	// 따라서 한 프레임 뒤에서 이벤트를 받아서 처리
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleMatchesAssignmentIfNotExpectingOne);
}

void ALeeGameModeBase::InitGameState()
{
	Super::InitGameState();

	
	
}

void ALeeGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

APawn* ALeeGameModeBase::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer,
	const FTransform& SpawnTransform)
{
	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);
}

void ALeeGameModeBase::HandleMatchesAssignmentIfNotExpectingOne()
{
}

bool ALeeGameModeBase::IsExperienceLoaded() const
{
	return false;
}

void ALeeGameModeBase::OnExperienceLoaded(const ULeeExperienceDefinition* CurrentExperience)
{
}
