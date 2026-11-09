// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility_BowBase.h"
#include "LeeGameplayAbility_BowFire.generated.h"

/**
 * 활 좌클릭 어빌리티.
 * 조준 중이면 조준 어빌리티에 발사 요청(ReleaseShot 이벤트)만 보내고,
 * 조준이 아니면 활을 최소한으로 당겨(손에 화살을 걸고 시위 당김) 약한 속사 화살을 쏜다.
 */
UCLASS(Abstract)
class GAS_PROJECT_API ULeeGameplayAbility_BowFire : public ULeeGameplayAbility_BowBase
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_BowFire(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 조준 없이 쏘는 속사 화살 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow")
	FLeeBowShotParams QuickShot = { 10.0f, 2500.0f, 2000.0f };

	/** 속사 때 활을 최소한으로 당기는 몽타주 (비우면 당기지 않고 바로 발사).
	 *  당긴 채 유지하는 Loop 섹션이 있는 몽타주는 스스로 끝나지 않으므로, 발사 시점은 QuickDrawTime이 정한다 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow")
	TObjectPtr<UAnimMontage> QuickDrawMontage;

	/** 당기기 시작 후 화살을 놓기까지 걸리는 시간(초) — 당기기 몽타주의 첫 구간 길이에 맞춘다 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow", meta = (ClampMin = "0.05", ForceUnits = "s"))
	float QuickDrawTime = 0.25f;

private:
	UFUNCTION()
	void OnFireMontageEnded();

	UFUNCTION()
	void OnQuickDrawFinished();

	UFUNCTION()
	void OnQuickDrawCancelled();

	/** 당기기가 끝난 속사 화살을 실제로 쏜다 (한 번만) */
	void ReleaseQuickShot();

	/** [서버] 속사로 손에 걸어 둔 화살 */
	UPROPERTY()
	TObjectPtr<ALeeArrowProjectile> HeldArrow;

	bool bQuickShotReleased = false;
};
