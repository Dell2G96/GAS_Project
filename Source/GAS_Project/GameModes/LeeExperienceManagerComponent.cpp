// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeExperienceManagerComponent.h"

#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"
#include "GameFeaturesSubsystemSettings.h"
#include "LeeExperienceActionSet.h"
#include "LeeExperienceDefinition.h"
#include "LeeExperienceManager.h"
#include "GAS_Project/LeeLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "GAS_Project/System/LeeAssetManager.h"

namespace LeeConsoleVariables
{
	static float ExperienceLoadRandomDelayMin = 0.0f;
	static FAutoConsoleVariableRef CVarExperienceLoadRandomDelayMin(
		TEXT("Lee.chaos.ExperienceDelayLoad.MinSecs"),
		ExperienceLoadRandomDelayMin,
		TEXT("This value (in seconds) will be added as a delay of load completion of the experience (along with the random value Lee.chaos.ExperienceDelayLoad.RandomSecs)"),
		ECVF_Default);

	static float ExperienceLoadRandomDelayRange = 0.0f;
	static FAutoConsoleVariableRef CVarExperienceLoadRandomDelayRange(
		TEXT("Lee.chaos.ExperienceDelayLoad.RandomSecs"),
		ExperienceLoadRandomDelayRange,
		TEXT("A random amount of time between 0 and this value (in seconds) will be added as a delay of load completion of the experience (along with the fixed value Lee.chaos.ExperienceDelayLoad.MinSecs)"),
		ECVF_Default);

	float GetExperienceLoadDelayDuration()
	{
		return FMath::Max(0.0f, ExperienceLoadRandomDelayMin + FMath::FRand() * ExperienceLoadRandomDelayRange);
	}
}

ULeeExperienceManagerComponent::ULeeExperienceManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}


void ULeeExperienceManagerComponent::SetCurrentExperience(FPrimaryAssetId ExperienceId)
{
	ULeeAssetManager& AssetManager = ULeeAssetManager::Get();
	FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(ExperienceId);
	TSubclassOf<ULeeExperienceDefinition> AssetClass = Cast<UClass>(AssetPath.TryLoad());
	check(AssetClass);
	const ULeeExperienceDefinition* Experience = GetDefault<ULeeExperienceDefinition>(AssetClass);

	check(Experience != nullptr);
	check(CurrentExperience == nullptr);
	CurrentExperience = Experience;
	StartExperienceLoad();
}

void ULeeExperienceManagerComponent::CallOrRegister_OnExperienceLoaded_HighPriority(
	FOnLeeExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded_HighPriority.Add(MoveTemp(Delegate));
	}
}


void ULeeExperienceManagerComponent::CallOrRegister_OnExperienceLoaded(FOnLeeExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded.Add(MoveTemp(Delegate));
	}
}


void ULeeExperienceManagerComponent::CallOrRegister_OnExperienceLoaded_LowPriority(
	FOnLeeExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded_LowPriority.Add(MoveTemp(Delegate));
	}
}

const ULeeExperienceDefinition* ULeeExperienceManagerComponent::GetCurrentExperienceChecked() const
{
	check(LoadState == ELeeExperienceLoadState::Loaded);
	check(CurrentExperience != nullptr);
	return CurrentExperience;
}

bool ULeeExperienceManagerComponent::IsExperienceLoaded() const
{
	return (LoadState == ELeeExperienceLoadState::Loaded) && (CurrentExperience != nullptr);

}

void ULeeExperienceManagerComponent::OnRep_CurrentExperience()
{
	StartExperienceLoad();
}


void ULeeExperienceManagerComponent::StartExperienceLoad()
{
	check(CurrentExperience);
	check(LoadState == ELeeExperienceLoadState::Unloaded);

	UE_LOG(LogLee, Log, TEXT("EXPERIENCE: StartExperienceLoad(CurrentExperience = %s, %s)"),
		*CurrentExperience->GetPrimaryAssetId().ToString(),
		*GetClientServerContextString(this));
	
	LoadState = ELeeExperienceLoadState::Loading;
	
	ULeeAssetManager& AssetManager = ULeeAssetManager::Get();

	TSet<FPrimaryAssetId> BundleAssetList;
	TSet<FSoftObjectPath> RawAssetList;
	
	BundleAssetList.Add(CurrentExperience->GetPrimaryAssetId());

	for (const TObjectPtr<ULeeExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
	{
		// LAS_Shooter_SharedHUD를 추가
		// BundleAssetList는 Bundle로 등록한 root의 PrimaryDataAsset을 추가하는 과정
		if (ActionSet)
		{
			BundleAssetList.Add(ActionSet->GetPrimaryAssetId());
		}
	}
	
	TArray<FName> BundlesToLoad;
	BundlesToLoad.Add(FLeeBundles::Equipped);

	
	{
		const ENetMode OwnerNetMode = GetOwner()->GetNetMode();
		bool bLoadClient = GIsEditor || (OwnerNetMode != NM_DedicatedServer ); 
		bool bLoadServer = GIsEditor || (OwnerNetMode != NM_Client );
		if (bLoadClient)
		{
			BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateClient);
		}
		if (bLoadServer)
		{
			BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateServer);
		}
	}

	TSharedPtr<FStreamableHandle> BundleLoadHandle = nullptr;
	if (BundleAssetList.Num() > 0)
	{
		BundleLoadHandle = AssetManager.ChangeBundleStateForPrimaryAssets(BundleAssetList.Array(), BundlesToLoad, {}, false, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority);
	}

	TSharedPtr<FStreamableHandle> RawLoadHandle = nullptr;
	if (RawAssetList.Num() > 0)
	{
		RawLoadHandle = AssetManager.LoadAssetList(RawAssetList.Array(), FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority, TEXT("StartExperienceLoad()"));
	}
	
	// TSharedPtr<FStreamableHandle> Handle = AssetManager.ChangeBundleStateForPrimaryAssets(
	// 	BundleAssetList.Array(),
	// 	BundlesToLoad,
	// 	{}, false, FStreamableDelegate(),
	// 	FStreamableManager::AsyncLoadHighPriority
	// 	);

	TSharedPtr<FStreamableHandle> Handle = nullptr;
	if (BundleLoadHandle.IsValid() && RawLoadHandle.IsValid())
	{
		Handle = AssetManager.GetStreamableManager().CreateCombinedHandle({BundleLoadHandle, RawLoadHandle});
	}
	else
	{
		Handle = BundleLoadHandle.IsValid() ? BundleLoadHandle : RawLoadHandle;
	}
	
	FStreamableDelegate OnAssetLoadedDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnExperienceLoadComplete);
	
	if (!Handle.IsValid() || Handle->HasLoadCompleted())
	{
		// 에셋이 이미 로드 되면, 델리게이트 호출
		FStreamableHandle::ExecuteDelegate(OnAssetLoadedDelegate);
	}
	else
	{
		Handle->BindCompleteDelegate(OnAssetLoadedDelegate);
		Handle->BindCancelDelegate(FStreamableDelegate::CreateLambda
			([OnAssetLoadedDelegate]()
			{
				OnAssetLoadedDelegate.ExecuteIfBound();
			}));
	}
	TSet<FPrimaryAssetId> PreloadAssetList;
	if (PreloadAssetList.Num() > 0)
	{
		AssetManager.ChangeBundleStateForPrimaryAssets(PreloadAssetList.Array(), BundlesToLoad, {});
	}


	// 삭제 예정
	// FTimerHandle DebugTimer;
	// GetWorld()->GetTimerManager().SetTimer(DebugTimer, FTimerDelegate::CreateLambda([this]()
	// {
	// 	UE_LOG(LogLee, Error, TEXT("FORCE OnExperienceLoadComplete! LoadState=%d"), (int32)LoadState);
	// 	if (LoadState == ELeeExperienceLoadState::Loading) {
	// 		OnExperienceLoadComplete();
	// 	}
	// }), 3.0f, false);  // 3초 후 강제 완료

	
	static int32 StartExPerienceLoad_FrameNumber = GFrameNumber;
}


void ULeeExperienceManagerComponent::OnExperienceLoadComplete()
{
	static int32 OnExperienceLoadComplete_FrameNumber = GFrameNumber;
	
	check(LoadState == ELeeExperienceLoadState::Loading);
	check(CurrentExperience);

	GameFeaturePluginURLs.Reset();

	auto CollectGameFeaturePluginURLs = [This = this](const UPrimaryDataAsset* Context, const TArray<FString>& FeaturePluginList)
	{
		for (const FString& PluginName : FeaturePluginList)
		{
			FString PluginURL;
			if (UGameFeaturesSubsystem::Get().GetPluginURLByName(PluginName, PluginURL))
			{
				This->GameFeaturePluginURLs.AddUnique(PluginURL);
			}
			else
			{
				ensureMsgf(false, TEXT("OnExperienceLoadComplete failed to find plugin URL from PluginName %s for experience %s - fix data, ignoring for this run"), *PluginName, *Context->GetPrimaryAssetId().ToString());		
			}
		}
	};

	CollectGameFeaturePluginURLs(CurrentExperience, CurrentExperience->GameFeaturesToEnable);
	for (const TObjectPtr<ULeeExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
	{
		if (ActionSet != nullptr)
		{
			CollectGameFeaturePluginURLs(ActionSet, ActionSet->GameFeaturesToEnable);
		}
	}

	NumGameFeaturePluginsloading = GameFeaturePluginURLs.Num();
	if (NumGameFeaturePluginsloading)
	{
		LoadState = ELeeExperienceLoadState::LoadingGameFeatures;
		for (const FString& PluginURL : GameFeaturePluginURLs)
		{
			UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(PluginURL, FGameFeaturePluginLoadComplete::CreateUObject(this, &ThisClass::OnGameFeaturePluginLoadComplete));
		}
	}
	else
	{
		OnExperienceFullLoadComplete();
	}
	
}


void ULeeExperienceManagerComponent::OnActionDeactivationCompleted()
{
	check(IsInGameThread());
	++NumObservedPausers;

	if (NumObservedPausers == NumExpectedPausers)
	{
		OnAllActionsDeactivated();
	}
}



void ULeeExperienceManagerComponent::OnGameFeaturePluginLoadComplete(const UE::GameFeatures::FResult& Result)
{ // 매 게임피처 플러그인이 로딩 될때 해당 함수가 콜 됨
	NumGameFeaturePluginsloading--;
	
	if (NumGameFeaturePluginsloading == 0)
	{
		// 게임피처 플러그인 로딩이 다 끝나면
		// Loaded 로 아래 함수 호출
		OnExperienceFullLoadComplete();
	}
}


void ULeeExperienceManagerComponent::OnExperienceFullLoadComplete()
{
	// check(LoadState != ELeeExperienceLoadState::Loaded);
	//
	// LoadState = ELeeExperienceLoadState::Loaded;
	// OnExperienceLoaded.Broadcast(CurrentExperience);
	// OnExperienceLoaded.Clear();


	check(LoadState != ELeeExperienceLoadState::Loaded);

	if (LoadState != ELeeExperienceLoadState::LoadingChaosTestingDelay)
	{
		const float DelaySecs = LeeConsoleVariables::GetExperienceLoadDelayDuration();
		if (DelaySecs > 0.f)
		{
			FTimerHandle DummyHandle;

			LoadState = ELeeExperienceLoadState::LoadingChaosTestingDelay;
			GetWorld()->GetTimerManager().SetTimer(DummyHandle, this, &ThisClass::OnExperienceFullLoadComplete, DelaySecs, false);

			return;
		}
	}
	
	// 게임 피처 플러그인 로딩과 활성화 이후, 게임피처 액션들을 활성화
	{
		LoadState = ELeeExperienceLoadState::ExecutingActions;

		FGameFeatureActivatingContext Context;
		{
			// 월드의 핸들 세팅
			const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
			if (ExistingWorldContext)
			{
				Context.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
			}
		}

		auto ActivateListOfActions = [&Context](const TArray<UGameFeatureAction*>& ActionList ){
				for (UGameFeatureAction* Action : ActionList)
				{
					// Register -> Loading -> Activating 순으로 호출
					if (Action)
					{
						Action->OnGameFeatureRegistering();
						Action->OnGameFeatureLoading();
						Action->OnGameFeatureActivating(Context);
					}	
				}
			};
		ActivateListOfActions(CurrentExperience->Actions);

		for (const TObjectPtr<ULeeExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
		{
			ActivateListOfActions(ActionSet->Actions);
		}
	}

	LoadState = ELeeExperienceLoadState::Loaded;

	OnExperienceLoaded_HighPriority.Broadcast(CurrentExperience);
	OnExperienceLoaded_HighPriority.Clear();
	
	OnExperienceLoaded.Broadcast(CurrentExperience);
	OnExperienceLoaded.Clear();

	OnExperienceLoaded_LowPriority.Broadcast(CurrentExperience);
	OnExperienceLoaded_LowPriority.Clear();
}


void ULeeExperienceManagerComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULeeExperienceManagerComponent, CurrentExperience);
}


void ULeeExperienceManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 이 경험이 로드한 모든 피처들을 비활성화합니다
	//@TODO: FILO 방식으로도 처리해야 합니다
	// for (const FString& PluginURL : GameFeaturePluginURLs)
	// {
	// 	if (ULeeExperienceManager::RequestToDeactivatePlugin(PluginURL))
	// 	{
	// 		UGameFeaturesSubsystem::Get().DeactivateGameFeaturePlugin(PluginURL);
	// 	}
	// }
	
	//@TODO: Ensure proper handling of a partially-loaded state too
	if (LoadState == ELeeExperienceLoadState::Loaded)
	{
		LoadState = ELeeExperienceLoadState::Deactivating;
	
		// Make sure we won't complete the transition prematurely if someone registers as a pauser but fires immediately
		NumExpectedPausers = INDEX_NONE;
		NumObservedPausers = 0;
	
		// Deactivate and unload the actions
		FGameFeatureDeactivatingContext Context(TEXT(""), [this](FStringView) { this->OnActionDeactivationCompleted(); });
	
		const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
		if (ExistingWorldContext)
		{
			Context.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
		}
	
		auto DeactivateListOfActions = [&Context](const TArray<UGameFeatureAction*>& ActionList)
		{
			for (UGameFeatureAction* Action : ActionList)
			{
				if (Action)
				{
					Action->OnGameFeatureDeactivating(Context);
					Action->OnGameFeatureUnregistering();
				}
			}
		};
	
		DeactivateListOfActions(CurrentExperience->Actions);
		for (const TObjectPtr<ULeeExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
		{
			if (ActionSet != nullptr)
			{
				DeactivateListOfActions(ActionSet->Actions);
			}
		}
	
		NumExpectedPausers = Context.GetNumPausers();
	
		if (NumExpectedPausers > 0)
		{
			UE_LOG(LogLee, Error, TEXT("Actions that have asynchronous deactivation aren't fully supported yet in Lee experiences"));
		}
	
		if (NumExpectedPausers == NumObservedPausers)
		{
			OnAllActionsDeactivated();
		}
	}
}

bool ULeeExperienceManagerComponent::ShouldShowLoadingScreen(FString& OutReason) const
{
	if (LoadState != ELeeExperienceLoadState::Loaded)
	{
		OutReason = TEXT("Experience Still Loading...");
		return true;
	}
	else
	{
		return false;
	}
}








void ULeeExperienceManagerComponent::OnAllActionsDeactivated()
{
	//@TODO: We actually only deactivated and didn't fully unload...
	LoadState = ELeeExperienceLoadState::Unloaded;
	CurrentExperience = nullptr;
	//@TODO:	GEngine->ForceGarbageCollection(true);
}


//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------
void ULeeExperienceManagerComponent::ServerSetCurrentExperience(FPrimaryAssetId ExperiencedId)
{
	ULeeAssetManager& AssetManager = ULeeAssetManager::Get();

	TSubclassOf<ULeeExperienceDefinition> AssetClass;
	{
		FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(ExperiencedId);
		AssetClass = Cast<UClass>(AssetPath.TryLoad());
	}
	// CDO ���·� ��������
	const ULeeExperienceDefinition* Experience = GetDefault<ULeeExperienceDefinition>(AssetClass);
	check(Experience != nullptr);
	check(CurrentExperience == nullptr);
	{
		CurrentExperience = Experience;
	}

	StartExperienceLoad();
}



