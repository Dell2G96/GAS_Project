// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/AEquipment/LeeGameplayAbility_FromEquipment.h"
#include "GAS_Project/AEquipment/LeeFinisherData.h"
#include "LeeGameplayAbility_Finisher.generated.h"

class UGameplayEffect;

/**
 * 처형/암살 통합 어빌리티 (공격자 측).
 *
 * 처형과 암살은 트리거 조건만 다르고 실행 흐름은 100% 공유한다.
 * 분기 지점은 정확히 2곳:
 *  ① FindFinisherTarget() — 타겟 상태 태그로 ELeeFinisherType 결정
 *     - Status.Groggy 보유 + 거리 이내                     → Execution
 *     - Status.Unaware 보유 + 후방 각도 이내 + 거리 이내    → Assassination
 *  ② FinisherData에서 ExecutionSet / AssassinationSet 선택
 *
 * 실행 흐름 (전부 공유):
 *  1) CommitAbility
 *  2) 서버에서 타겟 재검증 (UI 판정 결과를 신뢰하지 않음)
 *  3) GetAssociatedEquipment() → LeeMeleeWeaponInstance → FinisherData (무기별 데이터)
 *  4) GE_FinisherInvincible 적용 (핸들 보관, EndAbility에서 반드시 제거)
 *  5) Motion Warping 타겟 설정 (피해자 기준 AttackerOffset 위치로 정렬)
 *  6) SendGameplayEvent(Event.BeFinished) → 피해자의 GA_FinisherVictim 트리거
 *     (몽타주는 담지 않는다 — 피해자가 자기 스켈레톤 태그로 직접 조회한다. §GA_FinisherVictim 참고)
 *  7) PlayMontageAndWait(AttackerMontage)
 *  8) WaitGameplayEvent(Event.Finisher.Damage) — 몽타주 AnimNotify 타이밍에 데미지 GE 적용
 *  9) 완료/인터럽트 → EndAbility (무적 해제·워프 타겟 제거의 단일 정리 지점)
 *
 * 부여 경로: 무기의 EquipmentDefinition.AbilitySetsToGrant → 무기 장착 중에만 사용 가능.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_Finisher : public ULeeGameplayAbility_FromEquipment
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_Finisher(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

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

	/**
	 * 피해자가 스냅할 목표 Yaw. 공격자/피해자 양측이 같은 공식으로 계산해 정렬을 맞춘다.
	 *  - Execution: 피해자가 공격자를 바라봄 (정면 처형)
	 *  - Assassination: 피해자가 공격자 반대 방향을 바라봄 (후방 암살)
	 */
	static float ComputeVictimYaw(ELeeFinisherType Type, const FVector& AttackerLocation, const FVector& VictimLocation);

protected:
	/** 처형/암살 시퀀스 동안 공격자에게 적용할 무적 GE (Status.Invincible 부여). BP에서 GE_FinisherInvincible 지정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher|Effect")
	TSubclassOf<UGameplayEffect> InvincibleEffect;

	/** 피니셔 데미지 GE (SetByCaller 방식). BP에서 GE_FinisherDamage 지정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher|Effect")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** 실행 가능 거리 (cm). 이 거리 이내의 타겟만 피니셔 발동 가능 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher|Rule", meta = (ClampMin = "0.0"))
	float ExecuteRadius = 200.0f;

	/** 암살 허용 후방 각도 (도). 타겟 등 뒤를 중심으로 한 원뿔 전체 각 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher|Rule", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float BehindAngleDeg = 120.0f;

	/** AttackerMontage의 Motion Warping 윈도우가 참조할 워프 타겟 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Finisher|Rule")
	FName WarpTargetName = TEXT("FinisherWarp");

private:
	/** 실행 거리 내에서 피니셔 대상을 찾는다. 서버 권위 판정 — 분기 지점 ① */
	AActor* FindFinisherTarget(const FGameplayAbilityActorInfo* ActorInfo, ELeeFinisherType& OutType) const;

	/** AnimNotify(Event.Finisher.Damage) 수신 → 타겟에 데미지 GE 적용 */
	UFUNCTION()
	void OnDamageEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	/** 현재 시퀀스의 피해자 */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentTarget;

	/** 현재 시퀀스에서 사용 중인 애님 셋 (FinisherData에서 복사 보관) */
	UPROPERTY(Transient)
	FLeeFinisherAnimSet CurrentAnimSet;

	/** 분기 상태 — 요구사항의 bIsExecution 역할 */
	ELeeFinisherType CurrentType = ELeeFinisherType::Execution;

	/** 무적 GE 핸들. EndAbility에서 반드시 제거 */
	FActiveGameplayEffectHandle InvincibleHandle;

	/** ActivateAbility에서 부여한 상태 태그 (Status.Executing / Status.Assassinating). EndAbility에서 제거 */
	FGameplayTag AppliedStatusTag;
};
