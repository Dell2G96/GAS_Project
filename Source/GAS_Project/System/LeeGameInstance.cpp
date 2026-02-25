// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameInstance.h"

#include "LeeGameplayTags.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GAS_Project/MyTags.h"

void ULeeGameInstance::Init()
{
	Super::Init();

	UGameFrameworkComponentManager* ComponentManager = GetSubsystem<UGameFrameworkComponentManager>(this);
	if (ensure(ComponentManager))
	{
		// const FLeeGameplayTags& GameplayTags = FLeeGameplayTags::Get();

		ComponentManager->RegisterInitState(MyTags::InitState::Spawned, false , FGameplayTag());
		ComponentManager->RegisterInitState(MyTags::InitState::DataAvailable, false , MyTags::InitState::Spawned);
		ComponentManager->RegisterInitState(MyTags::InitState::DataInitialized, false , MyTags::InitState::DataAvailable);
		ComponentManager->RegisterInitState(MyTags::InitState::GameplayReady, false , MyTags::InitState::DataInitialized);
	}
}

void ULeeGameInstance::Shutdown()
{
	Super::Shutdown();
}
