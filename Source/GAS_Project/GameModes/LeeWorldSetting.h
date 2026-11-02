// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/WorldSettings.h"
#include "LeeWorldSetting.generated.h"




UCLASS()
class GAS_PROJECT_API ALeeWorldSetting : public AWorldSettings
{
	GENERATED_BODY()
public:
	ALeeWorldSetting(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	 virtual void CheckForErrors() override;
#endif

public:
	 FPrimaryAssetId GetDefaultGameplayExperience() const;

	// 맵에 기본 Experience가 지정돼 있는지 (ID 조회 성공 여부와 무관)
	bool HasDefaultGameplayExperience() const { return !DefaultGameplayExperience.IsNull(); }

protected:
	UPROPERTY(EditAnywhere, Category=GameMode)
	TSoftClassPtr<class ULeeExperienceDefinition> DefaultGameplayExperience;

#if WITH_EDITORONLY_DATA
	// Is this level part of a front-end or other standalone experience?
	// When set, the net mode will be forced to Standalone when you hit Play in the editor
	UPROPERTY(EditAnywhere, Category=PIE)
	bool ForceStandaloneNetMode = false;
#endif
	
};
