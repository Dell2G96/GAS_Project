// 26.11.08 신규 - 마법 Enemy(모르가나 등)의 가드 대신 쓰는 마법 쉴드

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_MagicShield.generated.h"

class UGameplayEffect;

/**
 * 마법 쉴드 어빌리티.
 * 켜진 동안(ShieldDuration) 모든 피해를 쉴드 HP로 흡수하고, 맞을 때마다 금 가는 Cue를 실행한다.
 * 쉴드 HP가 0이 되면 깨지는 Cue + 짧은 경직 + 재사용 대기. 퍼펙트 가드는 없다.
 * 피해 흡수 판정은 ULeeExecCalc_Damage(Status.Shield.Active)가, 피격 알림은 ULeeDefenseComponent가 한다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_MagicShield : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_MagicShield(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
	/** 쉴드 작동 시간(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield", meta = (ClampMin = "0.1"))
	float ShieldDuration = 2.0f;

	/** 쉴드 HP — 받은 피해만큼 줄고 0이 되면 깨진다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield", meta = (ClampMin = "1.0"))
	float ShieldMaxHP = 60.0f;

	/** 쉴드가 끝나거나 깨진 뒤 다시 쓸 수 있을 때까지의 시간(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield", meta = (ClampMin = "0.0"))
	float ShieldCooldown = 6.0f;

	/** (선택) 재사용 대기 GE (Duration = SetByCaller Souls.SetByCaller.Duration, Souls.Cooldown.MagicShield 부여).
	 *  비워 두면 Souls.Cooldown.MagicShield Loose 태그를 ShieldCooldown초 동안 붙인다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield|Effect")
	TSubclassOf<UGameplayEffect> ShieldCooldownEffect;

private:
	/** 쉴드 피격 이벤트 수신 → 쉴드 HP 감소 + 금 가는 Cue, 0이면 깨짐 */
	UFUNCTION()
	void OnShieldHitEventReceived(FGameplayEventData Payload);

	/** 작동 시간 종료 → 쉴드 해제 */
	UFUNCTION()
	void OnShieldDurationFinished();

	/** 남은 쉴드 비율(0~1)과 맞은 방향을 실어 피격 Cue를 실행한다 (0이면 Cue가 깨지는 이펙트를 낸다)
	 *  26.11.08 수정 - 맞은 방향(쉴드 중심 → 공격자, 월드)을 Normal로 넘겨 그 자리에 금이 생기게 한다 */
	void ExecuteShieldHitCue(float RemainingRatio, const FVector& HitDirection) const;

	/** 현재 쉴드 HP */
	float CurrentShieldHP = 0.0f;

	/** 이번 쉴드가 HP 0으로 깨졌는가 (종료 후 경직 이벤트용) */
	bool bShieldBroken = false;

	/** 마지막으로 쉴드를 때린 공격자 — 깨짐 경직의 방향 계산(Instigator)에 쓴다 */
	TWeakObjectPtr<AActor> LastAttacker;
};
