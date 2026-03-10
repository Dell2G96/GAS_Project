// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeWorldSetting.h"

#include "EngineUtils.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerStart.h"
#include "GAS_Project/LeeLogChannels.h"
#include "Misc/UObjectToken.h"

ALeeWorldSetting::ALeeWorldSetting(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

FPrimaryAssetId ALeeWorldSetting::GetDefaultGameplayExperience() const
{
	FPrimaryAssetId Result;
	if (!DefaultGameplayExperience.IsNull())
	{
		Result = UAssetManager::Get().GetPrimaryAssetIdForPath(DefaultGameplayExperience.ToSoftObjectPath());

		if (!Result.IsValid())
		{
			UE_LOG(LogLee, Error, TEXT("%s.DefaultGameplayExperience is %s but that failed to resolve into an asset ID (you might need to add a path to the Asset Rules in your game feature plugin or project settings"),
				*GetPathNameSafe(this), *DefaultGameplayExperience.ToString());
		}
	}
	return Result;
}

#if WITH_EDITOR


void ALeeWorldSetting::CheckForErrors()
{
	Super::CheckForErrors();

	FMessageLog MapCheck("MapCheck");

	for (TActorIterator<APlayerStart> PlayerStartIt(GetWorld()); PlayerStartIt; ++PlayerStartIt)
	{
		APlayerStart* PlayerStart = *PlayerStartIt;
		if (IsValid(PlayerStart) && PlayerStart->GetClass() == APlayerStart::StaticClass())
		{
			MapCheck.Warning()->AddToken(FUObjectToken::Create(PlayerStart))->AddToken(FTextToken::Create(FText::FromString("is a normal AplayerStart, replace with ALeePlayerstart")));
		}
	}
}
#endif

