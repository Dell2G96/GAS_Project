// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePlayerState.h"

#include "GAS_Project/GameModes/LeeExperienceManagerComponent.h"
#include "GAS_Project/GameModes/LeeGameModeBase.h"

void ALeePlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	const AGameStateBase* GameState = GetWorld()->GetGameState();
	check(GameState);

	ULeeExperienceManagerComponent* ExperienceManagerComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
	check(ExperienceManagerComponent);

	ExperienceManagerComponent->CallOrRegister_OnExperienceLoaded(FOnLeeExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void ALeePlayerState::OnExperienceLoaded(const class ULeeExperienceDefinition* CurrentExperience)
{
	if (ALeeGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ALeeGameModeBase>())
	{
		const ULeePawnData* NewPawnData = GameMode->GetPawnDataForController(GetOwningController());
		check(NewPawnData);

		SetPawnData(NewPawnData);
	}
}

void ALeePlayerState::SetPawnData(const class ULeePawnData* InPawnData)
{
	check(InPawnData);

	check(!PawnData);
	PawnData = InPawnData;
}