
#pragma once

#include "CoreMinimal.h"
#include "CGameplayAbility.h"
#include "GA_Combo.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Combo : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Combo();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
							const FGameplayAbilityActorInfo* ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo,
							bool bReplicateEndAbility, bool bWasCancelled) override;



	static FGameplayTag GetComboChangeEventTag();
	static FGameplayTag GetComboChangeEventEndTag();

private:
	// 입력 대기(연속 재설치)
	void SetupWaitComboInputPress();

	// 콤보 창
	UFUNCTION()
	void OnComboWindowOpened(FGameplayEventData Data);

	UFUNCTION()
	void OnComboWindowEnded(FGameplayEventData Data);

	// 입력 처리
	UFUNCTION()
	void OnInputPressed(float TimeWaited);

	// 유틸
	UAnimInstance* GetOwnerAnimInstance() const;


	
	UFUNCTION()
	void HandleInputPress(float TimeWaited);

	void TryCommitCombo();
	

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ComboMontage;

	UFUNCTION()
	void ComboChangedEventReceived(FGameplayEventData Data);
	
	FName NextComboName;
	// 창/섹션 상태
	bool  bComboWindowOpen = false;
	FName CandidateNextSection = NAME_None; // 창이 열릴 때 애님 노티로 들어온 후보(예: "M2")
	

};