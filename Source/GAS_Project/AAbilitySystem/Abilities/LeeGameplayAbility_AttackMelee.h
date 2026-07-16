// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_AttackMelee.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * Enemy 근접 공격 어빌리티 (신규, Lyra Clone 스타일).
 *
 * 기본 흐름:
 *  1) ActivateAbility -> CommitAbility
 *  2) ActivationOwnedTags에 의해 Souls.Status.Attack.Attacking 태그 자동 부여
 *  3) AttackMontages 중 1개 랜덤 선택 -> PlayMontageAndWait
 *  4) WaitGameplayEvent(MyTags::Abilities::Enemy::Trace): ANS_ToggleTrace가 발사하는 이벤트 대기
 *  5) 이벤트 수신 시 HitResult로 SetByCaller 방식 데미지 GE 적용
 *  6) 몽타주 완료/중단 -> EndAbility -> Attacking 태그 자동 제거
 *
 * 책임 분리:
 *  - StateTree Task (STT_MeleeAttack): 공격 타이밍 결정 + TryActivateAbilityByTag 호출
 *  - 본 Ability: 몽타주 재생 + 히트 판정 이벤트 수신 + 데미지 적용
 *  - ANS_ToggleTrace: 판정 구간에서 Sweep/Trace -> GameplayEvent로 HitResult 전송
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
	/** 공격 몽타주 풀. 1개 이상 등록. 활성화 시 랜덤 선택. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Montage")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	/** 히트 시 적용할 데미지 GE (SetByCaller 방식). 등 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/**
	 * 데미지 양. SetByCaller (MyTags::Souls::SetByCaller_Damage) 키로 GE에 전달.
	 * 음수로 변환되어 Health를 감소시킴.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Damage", meta = (ClampMin = "0.0"))
	float DamageAmount = 10.0f;

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

	/** 선택된 몽타주 보관 (디버깅 용도) */
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> SelectedMontage;
};
