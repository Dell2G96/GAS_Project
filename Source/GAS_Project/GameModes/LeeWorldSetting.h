// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/WorldSettings.h"
#include "LeeWorldSetting.generated.h"

#define UE_API GAS_PROJECT_API


UCLASS(MinimalAPI)
class ALeeWorldSetting : public AWorldSettings
{
	GENERATED_BODY()
public:
	UE_API ALeeWorldSetting(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	UE_API virtual void CheckForErrors() override;
#endif

public:
	UE_API FPrimaryAssetId GetDefaultGameplayExperience() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category=GameMode)
	TSoftClassPtr<class ULeeExperienceDefinition> DefaultGameplayExperience;

#if WITH_EDITORONLY_DATA
	// Is this level part of a front-end or other standalone experience?
	// When set, the net mode will be forced to Standalone when you hit Play in the editor
	UPROPERTY(EditDefaultsOnly, Category=PIE)
	bool ForceStandaloneNetMode = false;
#endif
	
};
#undef  UE_API