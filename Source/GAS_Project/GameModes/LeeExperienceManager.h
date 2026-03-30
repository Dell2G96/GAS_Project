// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "LeeExperienceManager.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeExperienceManager : public UEngineSubsystem
{
	GENERATED_BODY()
public:
#if WITH_EDITOR
	void OnPlayInEditorBegun();

	static void NotifyOfPluginActivation(const FString PluginURL);
	static bool RequestToDeactivatePlugin(const FString PluginURL);
#else
	static void NotifyOfPluginActivation(const FString PluginURL) {}
	static bool RequestToDeactivatePlugin(const FString PluginURL) { return true; }
#endif

private:
	TMap<FString, int32> GameFeaturePluginRequestCountMap;
	
};
