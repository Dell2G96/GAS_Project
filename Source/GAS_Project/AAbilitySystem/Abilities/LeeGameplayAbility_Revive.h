// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_Revive.generated.h"

class ULeeHealthComponent;

// 26.11.02 추가 - 넉다운된 동료를 일으키는 어빌리티. 상호작용(F)으로 부여·실행된다.
// ReviveDuration 동안 제자리 채널링 → 성공 시 대상에게 GameplayEvent.Revive 전송. 이동 입력·피격·대상 이탈 시 취소
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_Revive : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_Revive(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 채널링 시작 (BP에서 진행률 UI 표시용)
	UFUNCTION(BlueprintImplementableEvent, Category = "Lee|Revive", meta = (DisplayName = "OnReviveStarted"))
	void K2_OnReviveStarted(AActor* TargetActor, float Duration);

	// 채널링 종료 (bSucceeded=false면 취소됨)
	UFUNCTION(BlueprintImplementableEvent, Category = "Lee|Revive", meta = (DisplayName = "OnReviveEnded"))
	void K2_OnReviveEnded(bool bSucceeded);

	// 채널링 시간이 다 지남 → 서버에서 대상 부활
	UFUNCTION()
	void HandleReviveDelayFinished();

	// 주기적으로 취소 조건(이동 입력, 대상 사망/부활/거리 이탈) 검사
	void CheckCancelConditions();

	// 부활하는 사람이 피격되면 취소 (서버)
	UFUNCTION()
	void HandleReviverHealthChanged(ULeeHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator);

	// 대상이 아직 넉다운 상태인지
	bool IsTargetStillDowned() const;

protected:
	// 부활에 걸리는 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Revive", meta = (ClampMin = "0.1"))
	float ReviveDuration = 3.0f;

	// 이 거리보다 멀어지면 취소
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Revive", meta = (ClampMin = "0.0"))
	float MaxReviveDistance = 300.0f;

	// 부활하는 동안 재생할 몽타주 (없으면 생략)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Revive")
	TObjectPtr<UAnimMontage> ReviveMontage;

private:
	TWeakObjectPtr<AActor> ReviveTarget;

	FTimerHandle CancelCheckTimerHandle;

	bool bReviveSucceeded = false;

	// 26.11.06 추가 - [서버] 대상의 사망 카운트다운을 멈추도록 부활자로 등록했는지
	bool bAddedAsReviver = false;
};
