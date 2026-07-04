// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LeeFinisherData.generated.h"

class UAnimMontage;

/**
 * 피니셔 종류. 트리거 조건만 다를 뿐 GA_Finisher 하나가 둘 다 처리한다.
 * GameplayEventData::EventMagnitude로 피해자 측에 전달되므로 값 순서를 바꾸지 말 것.
 */
UENUM(BlueprintType)
enum class ELeeFinisherType : uint8
{
	Execution		UMETA(DisplayName = "처형 (그로기 대상)"),
	Assassination	UMETA(DisplayName = "암살 (미인식 대상)"),
};

/**
 * 처형/암살 한 종류에 대한 연출 데이터 묶음 (무기 쪽 = 공격자 전용).
 *
 * 피해자(Enemy) 몽타주는 여기 두지 않는다 — 무기 하나에 스켈레톤이 다른 Enemy가
 * 여러 종류 맞을 수 있어 "무기 × 스켈레톤" 조합 폭발이 발생하기 때문.
 * 대신 피해자 몽타주는 Enemy의 스켈레톤 태그로 ULeeFinisherVictimRegistry에서 조회한다
 * (§ACharacter/LeeFinisherVictimData.h, LeeFinisherVictimRegistry.h 참고).
 * 공격자/피해자 몽타주는 같은 기준(피해자 루트)으로 제작되어야 한다.
 */
USTRUCT(BlueprintType)
struct FLeeFinisherAnimSet
{
	GENERATED_BODY()

	/** 공격자(Player)가 재생할 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher")
	TObjectPtr<UAnimMontage> AttackerMontage = nullptr;

	/**
	 * 피해자 기준 공격자 정렬 트랜스폼 (Motion Warping 타겟 산출용).
	 * 예: 암살이면 피해자 등 뒤 = Location(-80, 0, 0), 회전 0.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher")
	FTransform AttackerOffset = FTransform(FVector(-80.0, 0.0, 0.0));

	/**
	 * 피니셔 데미지. SetByCaller(Souls.SetByCaller.Damage)로 GE_FinisherDamage에 전달.
	 * 즉사시키려면 MaxHealth 이상의 큰 값을 넣는다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher", meta = (ClampMin = "0.0"))
	float Damage = 999.0f;
};

/**
 * 무기별 처형/암살 데이터.
 * LeeMeleeWeaponInstance가 보유하며, GA_Finisher가 GetAssociatedEquipment()로 역조회한다.
 * → 무기를 바꾸면 어빌리티 코드 수정 없이 몽타주/데미지/정렬 오프셋이 교체된다.
 */
UCLASS(BlueprintType, Const)
class GAS_PROJECT_API ULeeFinisherData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher")
	FLeeFinisherAnimSet ExecutionSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher")
	FLeeFinisherAnimSet AssassinationSet;

	const FLeeFinisherAnimSet& GetAnimSet(ELeeFinisherType Type) const
	{
		return (Type == ELeeFinisherType::Execution) ? ExecutionSet : AssassinationSet;
	}
};
