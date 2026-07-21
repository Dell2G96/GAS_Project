// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_AttackMelee.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * 근접 공격 1개 엔트리 — 몽타주 + 이 몽타주 전용 데미지 + 공격 속성 태그.
 * 약공격/강공격은 이 구조체의 배열을 서로 다르게 채운 BP로 구분한다 (C++ 분기 없음).
 */
USTRUCT(BlueprintType)
struct FLeeMeleeAttackData
{
	GENERATED_BODY()

	/** 재생할 공격 몽타주 (단발 또는 다단히트 콤보 몽타주) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Montage = nullptr;

	/** 트레이스 이벤트 1회당 데미지. SetByCaller로 GE에 전달 (음수 변환은 적용 시점에 처리) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float BaseDamage = 10.0f;

	/**
	 * 이 몽타주에만 적용되는 공격 속성 태그.
	 * 데미지 GE Spec에 DynamicAssetTag로 실려 하위 시스템(강공격 피격 리액션, 가드 불가 판정 등)이 읽는다.
	 * 예: Souls.DamageType.Attack.Heavy, Souls.DamageType.Attack.Unblockable
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer DamageTypeTags;
};

/**
 * Enemy 근접 공격 어빌리티 (신규, Lyra Clone 스타일).
 *
 * 기본 흐름:
 *  1) ActivateAbility -> 공격 데이터 검증 -> CommitAbility
 *  2) ActivationOwnedTags에 의해 Souls.Status.Attack.Attacking 태그 자동 부여
 *  3) WaitGameplayEvent를 먼저 등록한 뒤, AttackDataList 중 1개 랜덤 선택 -> PlayMontageAndWait
 *  4) ANS_ToggleTrace가 발사하는 트레이스 이벤트 수신 시 HitResult로 SetByCaller 방식 데미지 GE 적용
 *  5) 몽타주 완료/중단 -> EndAbility -> Attacking 태그 자동 제거
 *
 * 책임 분리:
 *  - StateTree Task (FLeeStateTreeTask_MeleeAttack): 약/강 태그 선택 + TryActivateAbilitiesByTag 호출
 *  - 본 Ability: 몽타주 재생 + 히트 판정 이벤트 수신 + 데미지 적용
 *  - ANS_ToggleTrace: 판정 구간에서 Sweep/Trace -> GameplayEvent로 HitResult 전송
 *
 * 약공격/강공격 분리:
 *  - 이 C++ 클래스를 부모로 하는 BP 2개(예: GA_EnemyAttack_Light/Heavy)로 나눈다.
 *  - 각 BP가 AbilityTags에 고유 식별 태그(Souls.Abilities.Attack.Melee.Light/Heavy)를 추가하고,
 *    AttackDataList에 자신의 몽타주+데미지+속성 태그를 채운다. C++에는 약/강 분기 코드가 없다.
 *
 * 콤보(연속 공격):
 *  - 이번 단계는 별도의 콤보 진행 코드를 두지 않는다. AttackDataList에 단발 몽타주와
 *    다단히트 콤보 몽타주를 함께 등록하면, 랜덤 선택 결과에 따라 1회 공격 또는 연속 공격이
 *    자연스럽게 표현된다 (WaitGameplayEvent가 몽타주 1회 재생 동안의 모든 트레이스 이벤트를 수신).
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_AttackMelee : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_AttackMelee(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	/** 공격 데이터 풀 (몽타주+데미지+속성 태그). 1개 이상 등록. 활성화 시 랜덤 선택. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Montage")
	TArray<FLeeMeleeAttackData> AttackDataList;

	/** 히트 시 적용할 데미지 GE (SetByCaller 방식). 등 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/**
	 * ANS_ToggleTrace가 발사하는 이벤트 태그.
	 * 기본값: MyTags::Abilities::Enemy::Trace ("MyTags.Abilities.Enemy.Trace")
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Event")
	FGameplayTag TraceEventTag;

private:
	/** ANS_ToggleTrace로부터 HitResult 이벤트를 수신하여 데미지 GE 적용 */
	UFUNCTION()
	void OnTraceEventReceived(FGameplayEventData Payload);

	/** 몽타주 정상 완료 / 블렌드아웃 */
	UFUNCTION()
	void OnMontageCompleted();

	/** 몽타주 피격 취소 / 외부 취소 */
	UFUNCTION()
	void OnMontageInterrupted();

	/** 현재 활성화된 공격의 몽타주+데미지+속성 태그 (활성화 시 확정, 종료 시 초기화) */
	UPROPERTY(Transient)
	FLeeMeleeAttackData CurrentAttackData;
};
