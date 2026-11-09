// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility_BowBase.h"
#include "LeeGameplayAbility_BowAim.generated.h"

class UCameraShakeBase;

/** 활 차지 UI 메시지 (채널: Souls.Message.Bow.Charge) — 조작하는 플레이어 화면에서만 보낸다 */
USTRUCT(BlueprintType)
struct FLeeBowChargeMessage
{
	GENERATED_BODY()

	/** 차지 중인 캐릭터 */
	UPROPERTY(BlueprintReadOnly, Category = "Lee|Bow")
	TObjectPtr<AActor> Instigator = nullptr;

	/** true = 차지 시작, false = 차지 끝(발사·취소) */
	UPROPERTY(BlueprintReadOnly, Category = "Lee|Bow")
	bool bCharging = false;

	/** 차지를 시작한 월드 시간(초) */
	UPROPERTY(BlueprintReadOnly, Category = "Lee|Bow")
	float ChargeStartTime = 0.0f;

	/** 최대 차지까지 걸리는 시간(초) */
	UPROPERTY(BlueprintReadOnly, Category = "Lee|Bow")
	float MaxChargeTime = 1.0f;
};

/**
 * 활 조준·차지 어빌리티 (우클릭 Hold).
 * 우클릭을 누르는 동안 조준하고, 활을 다 당긴 뒤 누른 시간에 비례해 화살 데미지·속도·사거리가 커진다.
 * 조준 중 좌클릭(GA_BowFire가 보낸 ReleaseShot 이벤트)으로 발사, 최대 차지면 큰 카메라 쉐이크와 함께 자동 발사.
 * 우클릭을 떼면 쏘지 않고 취소. 발사 후에는 우클릭을 다시 눌러야 다음 화살을 당긴다.
 */
UCLASS(Abstract)
class GAS_PROJECT_API ULeeGameplayAbility_BowAim : public ULeeGameplayAbility_BowBase
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_BowAim(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 활을 당기는 몽타주 (몽타주 안의 SpawnArrow 노티파이 = 다 당김) */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow")
	TObjectPtr<UAnimMontage> DrawMontage;

	/** 다 당긴 뒤 최대 차지까지 걸리는 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow|Charge", meta = (ClampMin = "0.1", ForceUnits = "s"))
	float MaxChargeTime = 1.5f;

	/** 다 당기기 전에 좌클릭으로 쏜 약한 화살 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow|Charge")
	FLeeBowShotParams WeakShot = { 10.0f, 2500.0f, 2000.0f };

	/** 다 당긴 직후(차지 0%) 화살 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow|Charge")
	FLeeBowShotParams MinChargeShot = { 20.0f, 3500.0f, 3500.0f };

	/** 최대 차지(100%, 자동 발사) 화살 — 차지 비율에 따라 Min~Max 사이를 직선으로 보간 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow|Charge")
	FLeeBowShotParams MaxChargeShot = { 60.0f, 7000.0f, 8000.0f };

	/** 차지 중 흔들림 (세기 = 차지 비율 × ChargeShakeMaxScale) */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow|Shake")
	TSubclassOf<UCameraShakeBase> ChargeShakeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow|Shake", meta = (ClampMin = "0.0"))
	float ChargeShakeMaxScale = 1.0f;

	/** 최대 차지 자동 발사 순간의 큰 흔들림 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow|Shake")
	TSubclassOf<UCameraShakeBase> FullChargeShakeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Bow|Shake", meta = (ClampMin = "0.0"))
	float FullChargeShakeScale = 1.0f;

private:
	enum class EAimPhase : uint8
	{
		Drawing,	// 활을 당기는 중
		Charging,	// 다 당김, 차지 쌓는 중
		Fired		// 쐈음, 우클릭을 뗄 때까지 대기
	};

	UFUNCTION()
	void OnDrawMontageFinished();

	UFUNCTION()
	void OnSpawnArrowEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnReleaseShotEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnAimInputReleased(float TimeHeld);

	/** 다 당김 → 차지 시작 */
	void StartCharging();

	/** 최대 차지 도달 → 자동 발사 */
	void OnMaxChargeReached();

	/** 발사 (bFullCharge = 최대 차지 자동 발사) */
	void FireShot(bool bFullCharge);

	/** 현재 차지 비율 0~1 */
	float GetChargeRatio() const;

	/** 조준 중 캐릭터가 카메라 방향을 보도록 회전 방식 변경/복구 (락온 중이면 건드리지 않음) */
	void SetAimRotation(bool bAiming);

	void StartChargeShake();
	void UpdateChargeShake();
	void StopChargeShake();

	/** 차지 UI 메시지 전송 (로컬 플레이어만) */
	void BroadcastCharge(bool bCharging) const;

	EAimPhase Phase = EAimPhase::Drawing;
	float ChargeStartTime = 0.0f;
	bool bChangedOrientRotation = false;

	/** 조준이 바꾸기 전의 bOrientRotationToMovement 값 (조준 종료 때 되돌릴 값) */
	bool bOriginalOrientRotation = true;

	/** [서버] 손에 들고 있는 화살 */
	UPROPERTY()
	TObjectPtr<ALeeArrowProjectile> HeldArrow;

	/** [로컬] 재생 중인 차지 흔들림 */
	UPROPERTY()
	TObjectPtr<UCameraShakeBase> ChargeShakeInstance;

	FTimerHandle MaxChargeTimerHandle;
	FTimerHandle ShakeUpdateTimerHandle;
};
