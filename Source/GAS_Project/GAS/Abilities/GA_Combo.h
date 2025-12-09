#pragma once

#include "CoreMinimal.h"
#include "CGameplayAbility.h"
#include "GA_Combo.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UGA_Combo : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Combo();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	static FGameplayTag GetComboChangeEventTag();
	static FGameplayTag GetComboChangeEventEndTag();
	static FGameplayTag GetComboTargetEventTag();

public:
	//////////////////////////////////////////////////////////////////////
	//																	//
	//////////////////////////////////////////////////////////////////////
	UFUNCTION()
	void OnHitScanStartEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnHitScanEndEvent(FGameplayEventData Payload);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	FName StartSocket = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	FName EndSocket = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	float HitBoxRadius = 5.f;

	UFUNCTION(BlueprintCallable,  category="GAS|Combo")
	void SendHitReactEventToActors(const TArray<class AActor*>& HitActors);

	UFUNCTION(BlueprintCallable,  category="GAS|Combo")
	TArray<class AActor*> HitBoxTrace();

	UFUNCTION(BlueprintCallable,  category="GAS|Combo")
	void HitScanStart();

	UFUNCTION(BlueprintCallable,  category="GAS|Combo")
	void HitScanEnd();

	FTimerHandle HitBoxTraceTimerHandle;
	
	// 타이머에서 주기적으로 호출되는 함수
	UFUNCTION()
	void HitScanTick();
	
	void DrawDebugHitTrace(const TArray<FHitResult>& Hits, const FVector& HitBoxLocation) const;

	
protected:
	UPROPERTY()
	class ACPlayerCharacter* CachedOwnerCharacter = nullptr;

	UPROPERTY()
	class UCWeaponComponent* CachedWeaponComp = nullptr;

	UPROPERTY()
	class ACWeapon* CachedWeapon = nullptr;

	UPROPERTY()
	class USkeletalMeshComponent* CachedWeaponMesh = nullptr;

	// 이미 맞은 액터 (한 공격 창 동안 중복 히트 방지 + 불필요 처리 감소)
	TSet<TWeakObjectPtr<class AActor>> AlreadyHitActors;

	
private:
	void SetupWaitComboInputPress();

	UFUNCTION()
	void HandleInputPress(float TimeWaited);

	void TryCommitCombo();
	
	UPROPERTY(EditDefaultsOnly, Category="Targetting")
	float TargetSweepShpereRadius = 30.f;
	

	
	UPROPERTY(EditDefaultsOnly, Category="Gameplay Effect")
	TSubclassOf<UGameplayEffect> DefaultDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category="Gameplay Effect")
	class UGameplayEffect* DamageEffect;
	
	UPROPERTY(EditDefaultsOnly, Category="Gameplay Effect")
	TMap<FName, TSubclassOf<UGameplayEffect>> DamageEffectMap;

	TSubclassOf<UGameplayEffect> GetDamageEffectForCurrentCombo() const;

	
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	UAnimMontage* ComboMontage;

	UFUNCTION()
	void ComboChangedEventRecevied(FGameplayEventData Data);

	UFUNCTION()
	void DoDamageNew(FGameplayEventData Data);
	
	UFUNCTION()
	void DoDamage(FGameplayEventData Data);
	

	
	FName NextComboName;
};




