// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeAssetManager.h"

#include "GAS_Project/LeeLogChannels.h"

ULeeAssetManager::ULeeAssetManager()
{
}

ULeeAssetManager& ULeeAssetManager::Get()
{
	check(GEngine);

	if (ULeeAssetManager* Singleton = Cast<ULeeAssetManager>(GEngine->AssetManager) )
	{
		return *Singleton;	
	}

	UE_LOG(LogLee, Fatal, TEXT("invalid AssetManagerClassname in DefaultEngine.ini(project settings); it must be HakAssetManager"));
	
	return *NewObject<ULeeAssetManager>();
}

void ULeeAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	// ::InitializeNativeTags();

}

bool ULeeAssetManager::ShouldLogAssetLoads()
{
	const TCHAR* CommandLineContent = FCommandLine::Get();
	static bool bLogAssetLoads = FParse::Param(CommandLineContent, TEXT("LogAssetLoads"));
	return bLogAssetLoads;
}

// 동기식 로딩
UObject* ULeeAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	if (AssetPath.IsValid())
	{
		TUniquePtr<FScopeLogTime> LogTimerPtr;
		if (ShouldLogAssetLoads())
		{
			LogTimerPtr = MakeUnique<FScopeLogTime>(*FString::Printf(TEXT("synchronous loaded assets [%s]"), *AssetPath.ToString()), nullptr);;
		}

		if (UAssetManager::IsValid())
		{
			return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath);
		}
		return AssetPath.TryLoad();
	}
	return nullptr;
}
