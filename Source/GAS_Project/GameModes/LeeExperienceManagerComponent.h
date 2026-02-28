// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "GameFeaturesEditor/Private/GameFeatureDataDetailsCustomization.h"
#include "LeeExperienceManagerComponent.generated.h"

enum  class ELeeExperienceLoadState
{
	Unloaded,
	Loading,
	LoadingGameFeatures,
	ExecutingActions,
	Loaded,
	Deactivating,
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLeeExperienceLoaded, const class ULeeExperienceDefinition*)

UCLASS()
class GAS_PROJECT_API ULeeExperienceManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	bool IsExperienceLoaded()
	{
		return (LoadState == ELeeExperienceLoadState::Loaded)
		&& (CurrentExperience != nullptr);
	}

	void CallOrRegister_OnExperienceLoaded(FOnLeeExperienceLoaded::FDelegate&& Delegate);
	void ServerSetCurrentExperience(FPrimaryAssetId ExperiencedId);

	void StartExperiencedLoad();
	void OnExperienceLoadComplete();
	void OnGameFeaturePluginLoadComplete(const UE::GameFeatures::FResult& Result);
	void OnExperienceFullLoadComplete();

	const ULeeExperienceDefinition* GetCurrentExperienceChecked() const;
public:
	UPROPERTY()
	TObjectPtr<const ULeeExperienceDefinition> CurrentExperience;

	ELeeExperienceLoadState LoadState = ELeeExperienceLoadState::Unloaded;

	FOnLeeExperienceLoaded OnExperienceLoaded;

	int32 NumGameFeaturePluginsloading = 0;
	TArray<FString> GameFeaturePluginURLs;
	
};
