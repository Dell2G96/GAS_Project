// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GAS_Project/AEquipment/LeeFinisherData.h"
#include "LeeFinisherVictimData.generated.h"

class UAnimMontage;

/**
 * 스켈레톤(Enemy 종류)별 처형/암살 피해자 몽타주.
 *
 * 어떤 무기에 맞았는지와 무관하게 하나만 존재한다 — 쓰러지는 리액션은
 * 보통 무기 종류를 타지 않기 때문. 같은 스켈레톤을 쓰는 Enemy BP가
 * 여러 개(잡몹/엘리트 리스킨 등)여도 이 에셋 하나를 공유한다.
 *
 * ULeeFinisherVictimRegistry의 스켈레톤 태그 → 이 에셋 매핑을 통해 조회된다.
 */
UCLASS(BlueprintType, Const)
class GAS_PROJECT_API ULeeFinisherVictimData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher")
	TObjectPtr<UAnimMontage> ExecutionVictimMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher")
	TObjectPtr<UAnimMontage> AssassinationVictimMontage = nullptr;

	UAnimMontage* GetVictimMontage(ELeeFinisherType Type) const
	{
		return (Type == ELeeFinisherType::Execution) ? ExecutionVictimMontage : AssassinationVictimMontage;
	}
};
