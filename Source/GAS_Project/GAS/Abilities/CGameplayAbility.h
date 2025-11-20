// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Abilities/GameplayAbility.h"
#include "CGameplayAbility.generated.h"


UENUM(BlueprintType)
enum class AbilityActivationPolicy : uint8
{
	OnTriggered,
	OnGiven
};

UCLASS()
class GAS_PROJECT_API UCGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category="A|Ability")
	AbilityActivationPolicy ActivationPolicy = AbilityActivationPolicy::OnTriggered;

	int32 AbilityInputID;
	
	// ✅ 추가: AbilityInputID에 접근하기 위한 Getter
	FORCEINLINE int32 GetAbilityInputID() const { return AbilityInputID; }
	
	class UAnimInstance* GetOwnerAnimInstance() const;
	TArray<FHitResult> GetHitResultFromSweepLocationTargetData(
		const FGameplayAbilityTargetDataHandle& TargetDataHandle,
		float SphereSweepRadius = 30.f,
		ETeamAttitude::Type TargetTeam = ETeamAttitude::Hostile,
		bool bDrawDebug = false,
		bool bIgnoreSelf = true) const;
	
	// 디버그 옵션
	UFUNCTION(BlueprintCallable, Category="Debug")
	FORCEINLINE bool ShouldDrawDebug() const {return bShouldDrawDebug;}

protected:
	void PushSelf(const FVector& PushVel);
	void PushTarget(AActor* Target, const FVector& PushVel);
	void PushTargets(const TArray<AActor*>& Targets, const FVector& PushVel);
	void PushTargets(const FGameplayAbilityTargetDataHandle& TargetDataHandle , const FVector& PushVel);
	void PlayMontageLocally(UAnimMontage* MontageToPlay);
	void StopMontageAfterCurrentSection(UAnimMontage* MontageToStop);
	FGenericTeamId GetOwnerTeamID() const;

	ACharacter* GetOwningAvaterCharacter() ;
	void ApplyGameplayEffectToHitResultActor(const FHitResult& HitResult, TSubclassOf<UGameplayEffect> GameplayEffect, int Level = 1);
	void SendLocalGameplayEvent(const FGameplayTag& EventTag, const FGameplayEventData& EventData);

	
private:
	UPROPERTY(EditDefaultsOnly, Category="Debug")
	bool bShouldDrawDebug = false;

	UPROPERTY()
	class ACharacter* AvaterCharacter;
};


