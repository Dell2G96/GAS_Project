// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LeeFinisherVictimData.h"
#include "LeeFinisherVictimRegistry.generated.h"

/**
 * "Enemy 스켈레톤 태그" → "그 스켈레톤의 피해자 몽타주 데이터" 중앙 매핑 테이블.
 *
 * Enemy는 ULeeFinisherTargetComponent에 SkeletonTag 하나만 설정하면 되고(예:
 * Souls.Skeleton.Goblin), 실제 몽타주 데이터는 이 레지스트리 에셋 하나에서만
 * 관리한다. 신규 스켈레톤 추가 시 이 테이블에 한 줄만 추가하면 되고,
 * 같은 스켈레톤을 쓰는 Enemy BP가 몇 개든 태그만 맞으면 자동으로 동작한다.
 *
 * 싱글턴 접근: ULeeAssetManager::Get().GetFinisherVictimRegistry()
 * (LeeAssetManager의 DefaultPawnData와 동일한 TSoftObjectPtr + GetAsset 패턴)
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "Lee Finisher Victim Registry"))
class GAS_PROJECT_API ULeeFinisherVictimRegistry : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finisher")
	TMap<FGameplayTag, TObjectPtr<ULeeFinisherVictimData>> SkeletonVictimDataMap;

	const ULeeFinisherVictimData* FindVictimData(FGameplayTag SkeletonTag) const
	{
		if (const TObjectPtr<ULeeFinisherVictimData>* Found = SkeletonVictimDataMap.Find(SkeletonTag))
		{
			return *Found;
		}
		return nullptr;
	}
};
