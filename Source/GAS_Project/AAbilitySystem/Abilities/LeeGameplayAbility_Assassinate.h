// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_Assassinate.generated.h"

UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_Assassinate : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_Assassinate(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Assassination")
	UAnimMontage* AssassinationMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Assassination")
	TSubclassOf<class UGameplayEffect> InvincibleGE;

	UPROPERTY(EditDefaultsOnly, Category = "Assassination")
	TSubclassOf<class UGameplayEffect> AssassinatingStateGE;

	UPROPERTY(EditDefaultsOnly, Category = "Assassination")
	FName WarpTargetName = TEXT("AssassinationTarget");

	UPROPERTY(EditDefaultsOnly, Category = "Assassination")
	FGameplayTag AssassinationEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Assassination")
	FGameplayTag DeathEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Assassination", meta = (ClampMin = "0.0"))
	float ValidationRange = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Assassination", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float AssassinationAngle = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Assassination", meta = (ClampMin = "0.0"))
	float WarpDistanceBehindTarget = 100.0f;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	bool ValidateTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor) const;
	bool IsTargetUnaware(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor) const;
	bool IsBehindTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor) const;
	void SendDeathEventToTarget();

	FActiveGameplayEffectHandle ActiveInvincibleGEHandle;
	FActiveGameplayEffectHandle ActiveAssassinatingGEHandle;
	TWeakObjectPtr<AActor> CurrentVictim;
	bool bSentDeathEvent = false;
	bool bAddedAssassinatingTag = false;
};
