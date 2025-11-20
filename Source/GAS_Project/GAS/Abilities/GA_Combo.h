// GA_Combo.h
#pragma once

#include "CoreMinimal.h"
#include "CGameplayAbility.h"
#include "GA_Combo.generated.h"

UCLASS()
class GAS_PROJECT_API UGA_Combo : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Combo();

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	static FGameplayTag GetComboChangeEventTag();
	static FGameplayTag GetComboChangeEventEndTag();
	static FGameplayTag GetComboTargetEventTag();

private:
	// 입력을 이벤트로 받습니다(Pressed).
	UFUNCTION()
	void OnInputPressedEvent(struct FGameplayEventData Data);

	// 콤보 윈도우에서 섹션명 설정
	UFUNCTION()
	void ComboChangedEventRecevied(struct FGameplayEventData Data);

	// 대미지 예시(서버에서만)
	UFUNCTION()
	void DoDamage(struct FGameplayEventData Data);

	void TryCommitCombo();

	// 다음으로 넘어갈 섹션명
	FName NextComboName = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category="GameplayEffect")
	TSubclassOf<UGameplayEffect> DefaultDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category="GameplayEffect")
	TMap<FName, TSubclassOf<UGameplayEffect>> DamageEffectMap;

	UPROPERTY(EditDefaultsOnly, Category="Animation")
	UAnimMontage* ComboMontage = nullptr;
};
