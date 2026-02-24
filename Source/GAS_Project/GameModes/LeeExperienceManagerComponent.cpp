// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeExperienceManagerComponent.h"

#include "GameFeaturesSubsystemSettings.h"
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
		 
		// 복사비용을 낮추기 위해 Move를 통해 오른값 참조 사용
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
	// CDO 형태로 가져오기
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
		// 로딩이 완료되었으면 OnAssetLoadedDelegate호출
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
	static int32 OnExperienceLoadComplete_FrameNumber = GFrameNumber;
	OnExperienceFullLoadComplete();
}

void ULeeExperienceManagerComponent::OnExperienceFullLoadComplete()
{
	check(LoadState != ELeeExperienceLoadState::Loaded);

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