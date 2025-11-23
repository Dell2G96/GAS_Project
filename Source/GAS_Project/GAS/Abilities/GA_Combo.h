#pragma once

#include "CoreMinimal.h"
#include "CGameplayAbility.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Combo.generated.h"

class UAnimMontage;

UCLASS()
class GAS_PROJECT_API UGA_Combo : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Combo();

	// UGameplayAbility
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;


protected:
	// 태그 헬퍼
	FGameplayTag GetComboChangeEventTag() const;
	FGameplayTag GetComboChangeEventEndTag() const;

	UFUNCTION()
	void HandleInputPress(float TimeWaited);

	void TryCommitCombo();
	
	// 유틸
	void SetupWaitInputTask();
	
	UFUNCTION()
	void ComboChangedEventReceived(FGameplayEventData Data);



protected:
	UPROPERTY(EditDefaultsOnly, Category="Combo")
	UAnimMontage* ComboMontage = nullptr;

private:
	// 콤보 창 상태
	bool bComboWindowOpen = false;

	// 창이 열릴 때 태그에서 파싱한 "후보 섹션"
	FName CandidateNextSection = NAME_None;

	// 실제로 입력이 들어와 확정된 다음 섹션
	FName NextComboName = NAME_None;
};
