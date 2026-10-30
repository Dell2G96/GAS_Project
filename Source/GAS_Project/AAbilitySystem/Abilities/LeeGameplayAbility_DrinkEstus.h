#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_DrinkEstus.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * 에스트(회복약) 마시기 어빌리티 — 26.10.30 추가 (Phase 2-3)
 * Cost GE로 횟수 1 소모 → 몽타주 재생 → AnimNotify(Event.Estus.Heal) 시점에 회복 GE 적용.
 * Exclusive_Replaceable이라 피격(HitReaction)이 들어오면 자동 취소되고, 마시는 동안 구르기·공격·가드는 막힌다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_DrinkEstus : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_DrinkEstus(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void OnHealEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

protected:
	/** 마시는 몽타주 — 회복 시점에 Lee Gameplay Event 노티파이(Souls.Events.Estus.Heal)를 배치해야 한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Estus")
	TObjectPtr<UAnimMontage> DrinkMontage;

	/** 회복 GE. BP에서 GE_EstusHeal 지정 (Health += EstusHealAmount, Attribute Based) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Estus")
	TSubclassOf<UGameplayEffect> HealEffect;
};
