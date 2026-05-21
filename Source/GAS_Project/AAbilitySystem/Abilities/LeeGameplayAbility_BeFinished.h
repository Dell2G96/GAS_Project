// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "LeeGameplayAbility_BeFinished.generated.h"

class UGameplayEffect;
struct FLeeGameplayAbilityTargetData_Finish;

/**
 * 처형과 암살의 피해자 측 공통 어빌리티.
 * 공격자 GA가 GameplayEvent TargetData로 전달한 VictimMontage를 재생한다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_BeFinished : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_BeFinished(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Finish")
	TSubclassOf<UGameplayEffect> InvincibleGE;

	UPROPERTY(EditDefaultsOnly, Category = "Finish")
	TSubclassOf<UGameplayEffect> BeingFinishedStateGE;

	UPROPERTY(EditDefaultsOnly, Category = "Finish")
	FName VictimWarpTargetName = TEXT("FinishVictim");

	UPROPERTY(EditDefaultsOnly, Category = "Finish")
	FGameplayTag BeFinishedEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Finish", meta = (ClampMin = "0.0"))
	float VictimWarpDistanceFromAttacker = 80.0f;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

private:
	const FLeeGameplayAbilityTargetData_Finish* ExtractFinishData(const FGameplayEventData* TriggerEventData) const;
	void SetupVictimWarp(const FGameplayAbilityActorInfo* ActorInfo, const FTransform& AttackerWorldTransform) const;

	FActiveGameplayEffectHandle ActiveInvincibleGEHandle;
	FActiveGameplayEffectHandle ActiveBeingFinishedGEHandle;
	bool bAddedInvincibleTag = false;
};
