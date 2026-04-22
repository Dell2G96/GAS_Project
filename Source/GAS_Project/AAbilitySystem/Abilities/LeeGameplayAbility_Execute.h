// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_Execute.generated.h"

UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_Execute : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_Execute(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Execution")
	UAnimMontage* ExecutionMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Execution")
	TSubclassOf<class UGameplayEffect> InvincibleGE;

	UPROPERTY(EditDefaultsOnly, Category = "Execution")
	TSubclassOf<class UGameplayEffect> ExecutingStateGE;

	UPROPERTY(EditDefaultsOnly, Category = "Execution")
	TSubclassOf<class UGameplayEffect> ExecutionDamageGE;

	UPROPERTY(EditDefaultsOnly, Category = "Execution")
	FName WarpTargetName = TEXT("ExecutionTarget");

	UPROPERTY(EditDefaultsOnly, Category = "Execution")
	FGameplayTag ExecutionEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Execution")
	FGameplayTag DeathEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Execution", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExecutionDamageRatio = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Execution", meta = (ClampMin = "0.0"))
	float ValidationRange = 400.0f;
	
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	bool ValidateTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor) const;
	void TryApplyExecutionOutcomeToTarget();
	
	FActiveGameplayEffectHandle ActiveInvincibleGEHandle;
	FActiveGameplayEffectHandle ActiveExecutingGEHandle;

	TWeakObjectPtr<AActor> CurrentExecutionTarget;
	bool bSentDeathEvent = false;
	bool bAddedExecutingTag = false;
};
