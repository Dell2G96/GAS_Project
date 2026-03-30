// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeExperienceManager.h"

#if WITH_EDITOR

void ULeeExperienceManager::OnPlayInEditorBegun()
{
	ensure(GameFeaturePluginRequestCountMap.IsEmpty());
	GameFeaturePluginRequestCountMap.Empty();
}

void ULeeExperienceManager::NotifyOfPluginActivation(const FString PluginURL)
{
	if (GIsEditor)
	{
		ULeeExperienceManager* ExperienceManagerSubsystem = GEngine->GetEngineSubsystem<ULeeExperienceManager>();
		check(ExperienceManagerSubsystem);

		int32& Count = ExperienceManagerSubsystem->GameFeaturePluginRequestCountMap.FindOrAdd(PluginURL);
		++Count;
	}
}

bool ULeeExperienceManager::RequestToDeactivatePlugin(const FString PluginURL)
{
	if (GIsEditor)
	{
		ULeeExperienceManager* ExperienceManagerSubsystem =GEngine->GetEngineSubsystem<ULeeExperienceManager>();
		check(ExperienceManagerSubsystem);

		int32& Count = ExperienceManagerSubsystem->GameFeaturePluginRequestCountMap.FindChecked(PluginURL);
		--Count;

		if (Count ==0)
		{
			ExperienceManagerSubsystem->GameFeaturePluginRequestCountMap.Remove(PluginURL);
			return true;
		}

		return false;
		
	}
	return true;
}
#endif