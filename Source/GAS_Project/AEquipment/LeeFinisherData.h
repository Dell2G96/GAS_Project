// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LeeFinisherData.generated.h"

class UAnimMontage;
class ULeeCameraMode;

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
	 * 피해자 로컬 좌표계에서 본 공격자의 위치·회전 (Motion Warping 타겟 산출용, +X = 피해자가 바라보는 방향).
	 *
	 * 타입별 올바른 값 방향 (§26.07.08 세부계획서 3.1 표 참고):
	 *  - 암살(AssassinationSet): 피해자가 공격자 반대 방향을 봄 → Location(-80, 0, 0) = 등 뒤, 회전 Yaw 0°
	 *  - 처형(ExecutionSet):     피해자가 공격자를 봄          → Location(+120, 0, 0) = 정면 앞, 회전 Yaw 180° (공격자가 피해자를 마주봄)
	 *    ★ 처형에서 회전을 기본값(0°)으로 두면 공격자가 피해자와 같은 방향을 보고 서는 버그가 된다.
	 *
	 * 정확한 값 측정 절차 (몽타주 쌍마다 1회):
	 *  1) 빈 레벨에 SkeletalMeshActor 2개 — 피해자 메시를 원점(0,0,0), Yaw 0°에 배치
	 *  2) 각각 피해자/공격자 애니메이션의 동기 프레임(첫 타격 접촉)을 설정
	 *  3) 공격자 메시를 움직여 손/칼이 맞는 위치를 찾고, 그때의 위치·Yaw를 그대로 기록 (Z는 0)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher")
	FTransform AttackerOffset = FTransform(FVector(-80.0, 0.0, 0.0));

	/**
	 * true면 몽타주 시작 순간 공격자를 정렬 위치로 즉시 스냅한다 (Motion Warping 수렴 생략).
	 * 몽타주의 접근(워프) 구간이 매우 짧아 첫 프레임부터 완전 일치가 필요한 연출에만 켠다.
	 * 기본 false — 워프 수렴이 시각적으로 더 자연스럽다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher")
	bool bSnapAttackerAtStart = false;

	/**
	 * 이 피니셔 동안 사용할 카메라 모드 (예: CM_Finisher). None이면 카메라를 바꾸지 않는다.
	 * 공격자 로컬 화면에만 적용되며, 어빌리티 종료 시 기본 카메라로 자동 블렌딩 복귀한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher")
	TSubclassOf<ULeeCameraMode> CameraMode;

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
