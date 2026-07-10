// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/AAbilitySystem/Abilities/LeeGameplayAbility.h"
#include "LeeGameplayAbility_TargetLock.generated.h"

/**
 * 타겟 락온 토글 어빌리티.
 *
 * 이 어빌리티는 상태를 갖지 않는다 — ULeeTargetLockComponent::ToggleLock()을 호출하고
 * 즉시 종료되는 "명령"일 뿐이다 (Fire-and-forget). 락온 상태 유지, 후보 탐색, 카메라 전환,
 * 이동 플래그 변경은 전부 ULeeTargetLockComponent가 담당한다.
 *
 * ActivationGroup = Independent — 공격/구르기/피니셔와 상호 차단되지 않는다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_TargetLock : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_TargetLock(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
