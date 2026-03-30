// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LoadingProcessInterface.h"
#include "Components/GameStateComponent.h"
#include "LeeExperienceManagerComponent.generated.h"


namespace UE::GameFeatures { struct FResult; }

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLeeExperienceLoaded, const class ULeeExperienceDefinition*)

enum  class ELeeExperienceLoadState
{
	Unloaded,
	Loading,
	LoadingGameFeatures,
	LoadingChaosTestingDelay,
	ExecutingActions,
	Loaded,
	Deactivating,
};



UCLASS()
class GAS_PROJECT_API ULeeExperienceManagerComponent : public UGameStateComponent,
 public  ILoadingProcessInterface
{
	GENERATED_BODY()

public:
	ULeeExperienceManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//~ILoadingProcessInterface interface
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
	//~End of ILoadingProcessInterface

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	// 새로 추가 //
	 void SetCurrentExperience(FPrimaryAssetId ExperienceId);

	 bool IsExperienceLoaded() const;

	UFUNCTION()
	void OnRep_CurrentExperience();
	

	void OnActionDeactivationCompleted();
	void OnAllActionsDeactivated();

	void CallOrRegister_OnExperienceLoaded_HighPriority(FOnLeeExperienceLoaded::FDelegate&& Delegate);
	void CallOrRegister_OnExperienceLoaded(FOnLeeExperienceLoaded::FDelegate&& Delegate);
	void CallOrRegister_OnExperienceLoaded_LowPriority(FOnLeeExperienceLoaded::FDelegate&& Delegate);
	
	void ServerSetCurrentExperience(FPrimaryAssetId ExperiencedId);

	void StartExperienceLoad();
	void OnExperienceLoadComplete();
	void OnGameFeaturePluginLoadComplete(const UE::GameFeatures::FResult& Result);
	void OnExperienceFullLoadComplete();

	const ULeeExperienceDefinition* GetCurrentExperienceChecked() const;
public:
	UPROPERTY(ReplicatedUsing=OnRep_CurrentExperience)
	TObjectPtr<const ULeeExperienceDefinition> CurrentExperience;

	ELeeExperienceLoadState LoadState = ELeeExperienceLoadState::Unloaded;

	FOnLeeExperienceLoaded OnExperienceLoaded_HighPriority;
	
	FOnLeeExperienceLoaded OnExperienceLoaded;

	FOnLeeExperienceLoaded OnExperienceLoaded_LowPriority;

	int32 NumGameFeaturePluginsloading = 0;
	TArray<FString> GameFeaturePluginURLs;
	
	int32 NumObservedPausers = 0;
	int32 NumExpectedPausers = 0;
	
};
