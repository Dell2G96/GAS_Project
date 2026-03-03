// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameFeature_AddGameplayCuePaths.h"

#include "GameFeatureAction.h"
#include "GameFeatureAction_AddGameplayCuePath.h"
#include "GameFeatureData.h"
#include "GameplayCueSet.h"
#include "GAS_Project/AAbilitySystem/LeeGameplauCueManager.h"

ULeeGameplayFeaturePolicy::ULeeGameplayFeaturePolicy(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void ULeeGameplayFeaturePolicy::InitGameFeatureManager()
{
	Observers.Add(NewObject<ULeeGameFeature_AddGameplayCuePaths>());

	UGameFeaturesSubsystem& Subsystem = UGameFeaturesSubsystem::Get();
	for (UObject* Observer : Observers)
	{
		Subsystem.AddObserver(Observer);
	}
	
	Super::InitGameFeatureManager();
}

void ULeeGameplayFeaturePolicy::ShutdownGameFeatureManager()
{
	
	Super::ShutdownGameFeatureManager();

	UGameFeaturesSubsystem& Subsystem = UGameFeaturesSubsystem::Get();
	for (UObject* Observer : Observers)
	{
		Subsystem.RemoveObserver(Observer);
	}
	Observers.Empty();
}

void ULeeGameFeature_AddGameplayCuePaths::OnGameFeatureRegistering(const UGameFeatureData* GameFeatureData,
                                                                   const FString& PluginName, const FString& PluginURL)
{
	const FString PluginRootPath = TEXT("/") + PluginName;

	for (const UGameFeatureAction* Action : GameFeatureData->GetActions())
	{
		if (const UGameFeatureAction_AddGameplayCuePath* AddGameplayCueGFA = Cast<UGameFeatureAction_AddGameplayCuePath>(Action) )
		{
			const TArray<FDirectoryPath>& DirsToAdd = AddGameplayCueGFA->DirectoryPathsToAdd;

			//Todo 
			if (ULeeGameplayCueManager* GCM = ULeeGameplayCueManager::Get())
			{
				UGameplayCueSet* RuntimeGameplayCueSet = GCM->GetRuntimeCueSet();
				const int32 PreInitializeNumCues = RuntimeGameplayCueSet ? RuntimeGameplayCueSet->GameplayCueData.Num() : 0;

				for (const FDirectoryPath& Directory : DirsToAdd)
				{
					FString MutablePath = Directory.Path;

					UGameFeaturesSubsystem::FixPluginPackagePath(MutablePath, PluginRootPath, false);
					GCM->AddGameplayCueNotifyPath(MutablePath, false);
				}

				if (!DirsToAdd.IsEmpty())
				{
					GCM->InitializeEditorObjectLibrary();
				}

				const int32 PostInitializeNumCues = RuntimeGameplayCueSet ? RuntimeGameplayCueSet->GameplayCueData.Num() : 0;
				if (PreInitializeNumCues != PostInitializeNumCues)
				{
					GCM->RefreshGameplayCuePrimaryAsset();
				}
			}
		}
	}
}

void ULeeGameFeature_AddGameplayCuePaths::OnGameFeatureUnregistering(const UGameFeatureData* GameFeatureData,
	const FString& PluginName, const FString& PluginURL)
{
	const FString PluginRootPath = TEXT("/") + PluginName;
	for (const UGameFeatureAction* Action : GameFeatureData->GetActions())
	{
		if (const UGameFeatureAction_AddGameplayCuePath* AddGameplayCueGFA = Cast<UGameFeatureAction_AddGameplayCuePath>(GameFeatureData))
		{
			const TArray<FDirectoryPath>& DirsToAdd = AddGameplayCueGFA->DirectoryPathsToAdd;
			if (ULeeGameplayCueManager* GCM = ULeeGameplayCueManager::Get())
			{
				int32 NumRemoved = 0;
				for (const FDirectoryPath& Directory : DirsToAdd)
				{
					FString MutablePath = Directory.Path;
					UGameFeaturesSubsystem::FixPluginPackagePath(MutablePath, PluginRootPath, false);
					NumRemoved += GCM->RemoveGameplayCueNotifyPath(MutablePath, false);
					
				}
				ensure(NumRemoved == DirsToAdd.Num());
				if (NumRemoved > 0)
				{
					GCM->InitializeEditorObjectLibrary();
				}
				
			}
			
		}
	}
}
