// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "LeeExecCalc_Damage.generated.h"

/**
 * 데미지 판정 ExecCalc — "계산 전용"
 *
 * 방어자(Target)의 상태 태그를 우선순위대로 검사해 최종 Health/Stamina Modifier만 출력하고,
 * 판정 결과는 Spec의 DynamicAssetTags에 Souls.DamageResult.* 태그로 기록한다.
 * LeeSoulsStatSet::PostGameplayEffectExecute → OnDamageResolved → ULeeDefenseComponent가 담당.
 *
 * 판정 우선순위:
 *  1) Souls.Status.Invincible          → 데미지 0 (+ Dodge.Perfect 동시 보유 시 퍼펙트 회피 기록)
 *  2) Souls.Status.Guard.Perfect (전방) → 퍼펙트 가드: 데미지 0, 스태미나 소모 0
 *  3) Souls.Status.Guard.Active  (전방) → 일반 가드: HP 0, 스태미나 -GuardStaminaCost
 *  4) 없음 / 가드 각도 밖              → 일반 피격: 풀 데미지
 *
 * 가드 수치(GuardStaminaCost/GuardValidAngleDeg)는 방어자의 ULeeDefenseComponent에서 읽는다
 */

UCLASS()
class GAS_PROJECT_API ULeeExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	ULeeExecCalc_Damage();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
	/**
	 * 공격자가 방어자의 전방 가드 유효범위 안에 있는지 판정.
	 * GuardValidAngleDeg는 "전방 기준 좌우 각각 몇 도까지"를 뜻하는 half-angle이다
	 * (전체 각이 아님. 예: 45를 넣으면 정면 기준 ±45도, 총 90도 범위가 유효)
	 */
	static bool IsAttackInsideGuardArc(const AActor* Defender, const AActor* Attacker, float GuardValidAngleDeg);
};
