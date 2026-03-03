// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFeaturesProjectPolicies.h"
#include "GameFeatureStateChangeObserver.h"
#include "UObject/Object.h"
#include "LeeGameFeature_AddGameplayCuePaths.generated.h"

UCLASS()
class ULeeGameplayFeaturePolicy : public UDefaultGameFeaturesProjectPolicies
{
	GENERATED_BODY()
	
public:
	ULeeGameplayFeaturePolicy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitGameFeatureManager() override;
	virtual void ShutdownGameFeatureManager() override;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> Observers;
};

UCLASS()
class GAS_PROJECT_API ULeeGameFeature_AddGameplayCuePaths : public UObject, public IGameFeatureStateChangeObserver
{
	GENERATED_BODY()
public:
	virtual void OnGameFeatureRegistering(const UGameFeatureData* GameFeatureData, const FString& PluginName, const FString& PluginURL) override;
	virtual void OnGameFeatureUnregistering(const UGameFeatureData* GameFeatureData, const FString& PluginName, const FString& PluginURL) override;
};
