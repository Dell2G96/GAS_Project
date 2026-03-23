// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularGameState.h"

#include "Components/GameFrameworkComponentManager.h"
#include "Components/GameStateComponent.h"


// AModularGameStateBase //
void AModularGameStateBase::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);

}

void AModularGameStateBase::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);

	Super::BeginPlay();

}

void AModularGameStateBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);

	Super::EndPlay(EndPlayReason);

}


// AModularGameState //

void AModularGameState::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);

}

void AModularGameState::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);

	Super::BeginPlay();

}

void AModularGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);

	Super::EndPlay(EndPlayReason);

}

void AModularGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	TArray<UGameStateComponent*> ModularComponents;
	GetComponents(ModularComponents);
	for (UGameStateComponent* Component : ModularComponents)
	{
		Component->HandleMatchHasStarted();
	}

}
