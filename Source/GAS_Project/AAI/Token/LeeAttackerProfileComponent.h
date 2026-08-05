// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GAS_Project/AAI/Token/LeeAttackTokenComponent.h"
#include "LeeAttackerProfileComponent.generated.h"

/**
 * Enemy Pawn에 부착하는 공격 프로필 컴포넌트.
 * 각 공격 종류(약공격/강공격/원거리)의 Quota 태그·Cost를 노출하고,
 * Utility AI의 "마지막 사용 후 경과 시간" 계산에 쓰일 사용 시각을 기록한다.
 * (Q8 결정안: 강공격에 GAS Cooldown GE가 없으므로, 실제 게임플레이 제한과 무관한
 *  순수 AI 행동 편향 값으로 이 컴포넌트가 직접 TMap에 기록·조회한다.)
 */
UCLASS(Blueprintable, ClassGroup = (Lee), meta = (BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeAttackerProfileComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Lee|AttackToken")
	static ULeeAttackerProfileComponent* FindAttackerProfileComponent(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<ULeeAttackerProfileComponent>() : nullptr;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|AttackToken")
	FLeeAttackReservationConfig LightAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|AttackToken")
	FLeeAttackReservationConfig HeavyAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|AttackToken")
	FLeeAttackReservationConfig RangedAttack;

	/** 원거리 공격 아키타입 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|AttackToken")
	bool bCanRangedAttack = false;

	/** [서버] AttackTag의 사용 시각을 현재 시각으로 기록 (Claim 성공 시 호출) */
	UFUNCTION(BlueprintCallable, Category = "Lee|AttackToken")
	void RecordUse(FGameplayTag AttackTag);

	/** [서버] AttackTag의 마지막 사용 후 경과 시간(초). 한 번도 사용하지 않았으면 매우 큰 값을 반환 */
	UFUNCTION(BlueprintPure, Category = "Lee|AttackToken")
	double GetTimeSinceLastUse(FGameplayTag AttackTag) const;

private:
	/** [서버 전용] 공격 태그별 마지막 사용 시각 */
	TMap<FGameplayTag, double> LastUseTime;
};
