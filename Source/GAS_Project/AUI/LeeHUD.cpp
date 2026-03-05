// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeHUD.h"

#include "Components/GameFrameworkComponentManager.h"

ALeeHUD::ALeeHUD(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void ALeeHUD::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void ALeeHUD::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
	Super::BeginPlay();
}

void ALeeHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	
	Super::EndPlay(EndPlayReason);
}
