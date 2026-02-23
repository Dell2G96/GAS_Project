// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "LeeExperienceManagerComponent.generated.h"

enum  class ELeeExperienceLoadState
{
	Unloaded,
	Loading,
	Loaded,
	Deactivating,
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLeeExperienceLoaded, const class ULeeExperienceDefinition*)

UCLASS()
class GAS_PROJECT_API ULeeExperienceManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	bool IsExperienceLoaded() { return (LoadState == ELeeExperienceLoadState::Loaded) && (CurrentExperience != nullptr); }

	void CallorRegister_OnExperienceLoaded(FOnLeeExperienceLoaded::FDelegate&& Delegate);
	
public:
	UPROPERTY()
	TObjectPtr<const ULeeExperienceDefinition> CurrentExperience;

	ELeeExperienceLoadState LoadState = ELeeExperienceLoadState::Unloaded;

	FOnLeeExperienceLoaded OnExperienceLoaded;
	
};
