// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "LeeFinishInteractionComponent.h"
#include "LeeGameplayAbilityTargetData_Finish.generated.h"

class UAnimMontage;

/**
 * 공격자 GA가 선택한 피해자 몽타주와 동기화 기준 Transform을 피해자 GA로 전달한다.
 */
USTRUCT()
struct FLeeGameplayAbilityTargetData_Finish : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY()
	FName PairID = NAME_None;

	UPROPERTY()
	TObjectPtr<UAnimMontage> VictimMontage = nullptr;

	UPROPERTY()
	FTransform AttackerWorldTransform = FTransform::Identity;

	UPROPERTY()
	ELeeFinishType FinishType = ELeeFinishType::None;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}

	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("PairID=%s, FinishType=%d"), *PairID.ToString(), static_cast<int32>(FinishType));
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FLeeGameplayAbilityTargetData_Finish> : public TStructOpsTypeTraitsBase2<FLeeGameplayAbilityTargetData_Finish>
{
	enum
	{
		WithNetSerializer = true
	};
};
