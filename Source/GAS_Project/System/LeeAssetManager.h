// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameData.h"
#include "Engine/AssetManager.h"
#include "GAS_Project/ACharacter/LeePawnData.h"
#include "LeeAssetManager.generated.h"

/**
 * 
 */
UCLASS(Config = Game)
class GAS_PROJECT_API ULeeAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	ULeeAssetManager();

	static ULeeAssetManager& Get();

	template <typename AssetType>
static AssetType* GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	template <typename AssetType>
	static TSubclassOf<AssetType> GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);
	
	// 26.03.19 추가
	const ULeeGameData& GetGameData();
	const ULeePawnData* GetDefaultPawnData() const;

protected:
	template <typename GameDataClass>
	const GameDataClass& GetOrLoadTypedGameData(const TSoftObjectPtr<GameDataClass>& DataPath)
	{
		if (TObjectPtr<UPrimaryDataAsset> const* pResult = GameDataMap.Find(GameDataClass::StaticClass()))
		{
			return *CastChecked<GameDataClass>(*pResult);
		}

		return * CastChecked<const GameDataClass>(LoadGameDataOfClass(GameDataClass::StaticClass(), DataPath, GameDataClass::StaticClass()->GetFName()));
	}

	UPrimaryDataAsset* LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType);





	
public:
	virtual void StartInitialLoading() final;

	static bool ShouldLogAssetLoads();

	static UObject* SynchronousLoadAsset(const FSoftObjectPath& AssetPath);



	/** [THREAD-SAFE] 메모리에 로딩된 에셋 캐싱 */
	void AddLoadedAsset(const UObject* Asset);

	// GC의 대상
	UPROPERTY()
	TSet<TObjectPtr<const UObject>> LoadedAssets;

	// Object 단위 Locking
	FCriticalSection LoadedAssetsCritical;


protected:
	UPROPERTY(Config)
	TSoftObjectPtr<ULeeGameData> LeeGameDataPath;
	
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass> , TObjectPtr<UPrimaryDataAsset>> GameDataMap;

	UPROPERTY(Config)
	TSoftObjectPtr<ULeePawnData> DefaultPawnData;
};

template <typename AssetType>
AssetType* ULeeAssetManager::GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepsInMemory)
{
	AssetType* LoadedAsset = nullptr;
	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		// 로딩이 되어있다? -> 바로 가져옴
		// 로딩이 안되어 있다 -> Null
		LoadedAsset = AssetPointer.Get();
		if (!LoadedAsset)
		{
			LoadedAsset = Cast<AssetType>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedAsset, TEXT("Failed to load asset [%s]"), *AssetPointer.ToString());
		}

		if (LoadedAsset && bKeepsInMemory)
		{
			// 여기서 AddLoadAsset은 메모리에 상주하기 위한 장치라고 생각하면 됨:
			// - 한번 등록되면 직접 내리지 않는한 Unload가 되지 않음 (== 캐싱)
			Get().AddLoadedAsset(Cast<UObject>(LoadedAsset));
		}
	}

	return LoadedAsset;
}

template <typename AssetType>
TSubclassOf<AssetType> ULeeAssetManager::GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	TSubclassOf<AssetType> LoadedSubclass;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();
	if (AssetPath.IsValid())
	{
		LoadedSubclass = AssetPointer.Get();
		if (!LoadedSubclass)
		{
			LoadedSubclass = Cast<UClass>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedSubclass, TEXT("Failed to load asset class [%s]"), *AssetPointer.ToString());
		}

		if (LoadedSubclass && bKeepInMemory)
		{
			Get().AddLoadedAsset(Cast<UObject>(LoadedSubclass));
		}
	}

	return LoadedSubclass;
}

