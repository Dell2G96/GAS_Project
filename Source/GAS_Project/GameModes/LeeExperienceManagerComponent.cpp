// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeExperienceManagerComponent.h"

#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"
#include "GameFeaturesSubsystemSettings.h"
#include "LeeExperienceActionSet.h"
#include "LeeExperienceDefinition.h"
#include "GAS_Project/System/LeeAssetManager.h"

void ULeeExperienceManagerComponent::CallOrRegister_OnExperienceLoaded(FOnLeeExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		 
		// �������� ���߱� ���� Move�� ���� ������ ���� ���
		OnExperienceLoaded.Add(MoveTemp(Delegate));
	}
}

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

	StartExperiencedLoad();
}

void ULeeExperienceManagerComponent::StartExperiencedLoad()
{
	check(CurrentExperience);
	check(LoadState == ELeeExperienceLoadState::Unloaded);

	LoadState = ELeeExperienceLoadState::Loading;
	ULeeAssetManager& AssetManager = ULeeAssetManager::Get();

	TSet<FPrimaryAssetId> BundleAssetList;
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

	FStreamableDelegate OnAssetLoadedDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnExperienceLoadComplete);

	TSharedPtr<FStreamableHandle> Handle = AssetManager.ChangeBundleStateForPrimaryAssets(
		BundleAssetList.Array(),
		BundlesToLoad,
		{}, false, FStreamableDelegate(),
		FStreamableManager::AsyncLoadHighPriority
		);

	if (!Handle.IsValid() || Handle->HasLoadCompleted())
	{
		// �ε��� �Ϸ�Ǿ����� OnAssetLoadedDelegateȣ��
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
	static int32 StartExPerienceLoad_FrameNumber = GFrameNumber;
}

void ULeeExperienceManagerComponent::OnExperienceLoadComplete()
{
	
	// OnExperienceFullLoadComplete();
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
		}
	};

	CollectGameFeaturePluginURLs(CurrentExperience, CurrentExperience->GameFeaturesToEnable);

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
	OnExperienceLoaded.Broadcast(CurrentExperience);
	OnExperienceLoaded.Clear();
}

const ULeeExperienceDefinition* ULeeExperienceManagerComponent::GetCurrentExperienceChecked() const
{
	check(LoadState == ELeeExperienceLoadState::Loaded);
	check(CurrentExperience != nullptr);
	return CurrentExperience;
}