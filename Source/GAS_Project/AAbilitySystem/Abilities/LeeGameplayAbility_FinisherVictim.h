// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_FinisherVictim.generated.h"

/**
 * 처형/암살 피해자 어빌리티 (Enemy 측).
 *
 * 트리거: GameplayEvent(Souls.Events.Finish.BeFinished) — GA_Finisher가 전송.
 * Payload:
 *  - Instigator       : 공격자 액터
 *  - EventMagnitude   : ELeeFinisherType (0=처형, 1=암살) — 회전 정렬 방향 + 몽타주 선택 기준
 *
 * 몽타주는 공격자가 전달하지 않는다. 무기(공격자) 하나에 스켈레톤이 다른 Enemy가
 * 여러 종류 맞을 수 있어 "무기 × 스켈레톤" 조합 폭발을 피하기 위해, 피해자가
 * 자기 자신의 스켈레톤 태그(ULeeFinisherTargetComponent::SkeletonTag)로
 * ULeeFinisherVictimRegistry에서 직접 몽타주를 조회한다.
 *
 * 책임:
 *  1) ActivationOwnedTags로 Status.Finisher.Victim 자동 부여/해제
 *     (다른 공격자의 중복 피니셔 잠금 + AI가 태그로 반응)
 *  2) 자신의 SkeletonTag로 ULeeFinisherVictimRegistry에서 VictimData 조회 → 몽타주 선택
 *  3) 공격자와 동일한 공식(ComputeVictimYaw)으로 회전 스냅 → 몽타주 정렬 동기화
 *  4) VictimMontage 재생. 인터럽트 포함 모든 종료가 EndAbility로 수렴 → 태그 정리 보장
 *
 * 데미지/사망 처리는 공격자 측(GA_Finisher의 AnimNotify 타이밍)이 담당한다.
 * Enemy 기본 AbilitySet에 포함해 스폰 시 부여할 것.
 * Enemy BP에는 ULeeFinisherTargetComponent가 부착되어 SkeletonTag가 설정되어 있어야 한다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_FinisherVictim : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_FinisherVictim(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();
};
