// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeFinishInteractionComponent.h"
#include "LeeFinishMontagePair.generated.h"

class UAnimMontage;

/**
 * 공격자 몽타주와 피해자 몽타주를 하나의 처형/암살 페어로 묶는 데이터.
 * ULeeWeaponInstance 서브클래스에서 무기별로 편집한다.
 */
USTRUCT(BlueprintType)
struct FLeeFinishMontagePair
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	FName PairID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	TObjectPtr<UAnimMontage> AttackerMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	TObjectPtr<UAnimMontage> VictimMontage = nullptr;

	// 문서 공식 기준: TargetLocation - TargetForward * X + TargetRight * Y.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	FVector AttackerWarpOffset = FVector(80.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageRatio = 1.0f;

	bool IsValidPair() const
	{
		return AttackerMontage != nullptr && VictimMontage != nullptr;
	}
};
