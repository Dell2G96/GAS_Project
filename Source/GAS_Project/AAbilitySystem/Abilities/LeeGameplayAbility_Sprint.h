#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_Sprint.generated.h"

class UGameplayEffect;
class UInputAction;

/**
 * 26.11.17 추가 - 달리기 어빌리티. 회피(스페이스)를 길게 누르면 회피 어빌리티가 끝나면서 이 어빌리티를 켠다.
 * 켜져 있는 동안 Status.Sprinting 태그로 이동 속도가 빨라지고(LeeCharacterMovementComponent), 이동 중에는 초당 스태미나를 쓴다.
 * 키를 떼거나 스태미나가 0이 되면 끝난다. 공격·회피·가드 등 다른 Exclusive 어빌리티가 켜지면 자동으로 취소된다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_Sprint : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_Sprint(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 누르고 있는지 확인할 입력 액션 (예: IA_Dodge_Souls). 떼면 달리기가 끝난다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Sprint")
	TObjectPtr<const UInputAction> SprintInputAction;

	/** 달리는 동안(실제로 움직일 때만) 초당 소모하는 스태미나 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Sprint", meta = (ClampMin = "0.0"))
	float StaminaCostPerSecond = 10.0f;

	/** 스태미나 차감 GE (Instant, SetByCaller Souls.SetByCaller.StaminaDamage). BP에서 GE_AttackStaminaCost 지정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Sprint")
	TSubclassOf<UGameplayEffect> StaminaCostEffect;

	/** 이 속도(cm/s)보다 느리면 제자리로 보고 스태미나를 쓰지 않는다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Sprint", meta = (ClampMin = "0.0"))
	float MinMoveSpeedForCost = 50.0f;

	/** 달리기 종료 후 스태미나 회복을 늦추는 GE (Duration = SetByCaller Souls.SetByCaller.Duration). BP에서 GE_StaminaRegenDelay 지정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Sprint")
	TSubclassOf<UGameplayEffect> StaminaRegenDelayEffect;

	/** 이 태그 중 하나라도 붙으면 달리기를 끝낸다 (Independent 그룹이라 달리기를 자동 취소하지 못하는 가드·에스트 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Sprint")
	FGameplayTagContainer StopSprintTags;

	/** 달리기 종료 후 스태미나 회복 지연 시간(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Sprint", meta = (ClampMin = "0.0"))
	float StaminaRegenDelayDuration = 1.0f;

private:
	/** 0.1초마다 — 키 뗌·스태미나 0 확인, [서버] 이동 중이면 스태미나 차감 */
	void TickSprint();

	/** 현재 스태미나 */
	float GetCurrentStamina() const;

	/** 확인 주기(초) */
	static constexpr float TickInterval = 0.1f;

	FTimerHandle TickTimerHandle;
};
