// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/AAbilitySystem/LeeFinishMontagePair.h"
#include "GAS_Project/AEquipment/LeeGameplayAbility_FromEquipment.h"
#include "GameplayTagContainer.h"
#include "LeeGameplayAbility_Finish.generated.h"

class UGameplayEffect;
class ULeeWeaponInstance;

/**
 * 처형과 암살의 공격자 측 공통 어빌리티.
 * 현재 장착 무기 인스턴스에서 몽타주 페어를 선택하고, 피해자에게 BeFinished 이벤트를 전달한다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_Finish : public ULeeGameplayAbility_FromEquipment
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_Finish(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Finish")
	TSubclassOf<UGameplayEffect> InvincibleGE;

	UPROPERTY(EditDefaultsOnly, Category = "Finish")
	TSubclassOf<UGameplayEffect> FinishingStateGE;

	UPROPERTY(EditDefaultsOnly, Category = "Finish")
	TSubclassOf<UGameplayEffect> FinishDamageGE;

	UPROPERTY(EditDefaultsOnly, Category = "Finish")
	FName AttackerWarpTargetName = TEXT("FinishAttacker");

	UPROPERTY(EditDefaultsOnly, Category = "Finish")
	FGameplayTag ExecutionEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Finish")
	FGameplayTag LegacyExecutionEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Finish")
	FGameplayTag AssassinationEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Finish")
	FGameplayTag BeFinishedEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Finish")
	FGameplayTag DeathEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Finish", meta = (ClampMin = "0.0"))
	float ValidationRange = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Finish", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float AssassinationAngle = 60.0f;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

private:
	ELeeFinishType ResolveFinishType(const FGameplayEventData* TriggerEventData) const;
	bool ValidateTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor, ELeeFinishType Type) const;
	bool IsTargetUnaware(AActor* TargetActor) const;
	bool IsBehindTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor) const;
	ULeeWeaponInstance* FindWeaponInstance(const FGameplayAbilityActorInfo* ActorInfo) const;
	FTransform BuildAttackerWarpTransform(const AActor* TargetActor, const FLeeFinishMontagePair& Pair) const;
	void SetupAttackerWarp(const FGameplayAbilityActorInfo* ActorInfo, const FTransform& WarpTransform) const;
	void SendBeFinishedEvent(AActor* TargetActor, const FLeeFinishMontagePair& Pair, const FTransform& AttackerWarpTransform);
	void ApplyFinishOutcomeToTarget();
	void RemoveTargetPromptEffects(UAbilitySystemComponent* TargetASC) const;

	FActiveGameplayEffectHandle ActiveInvincibleGEHandle;
	FActiveGameplayEffectHandle ActiveFinishingGEHandle;
	TWeakObjectPtr<AActor> CurrentFinishTarget;
	FLeeFinishMontagePair CurrentPair;
	ELeeFinishType CurrentFinishType = ELeeFinishType::None;
	bool bAddedFinishingTag = false;
	bool bSentOutcome = false;
};
