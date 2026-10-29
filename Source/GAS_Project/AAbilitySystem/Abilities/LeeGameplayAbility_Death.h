// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_Death.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_Death : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_Death(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category="Lee|Ability")
	void StartDeath();
	
	UFUNCTION(BlueprintCallable, Category="Lee|Ability")
	void FinishDeath();

	// 26.10.29 추가 - HealthComponent의 사망 완료(FinishDeath) 시 어빌리티를 종료한다
	// 종료하지 않으면 Exclusive_Blocking 그룹이 PlayerState ASC에 남아 부활한 Pawn의 어빌리티를 막는다
	UFUNCTION()
	void HandleDeathFinished(AActor* OwningActor);

	// 26.10.29 추가 - 사망 이벤트(OptionalObject2)에 실린 사망 몽타주를 ASC로 재생 (멀티에서 다른 클라이언트에도 복제됨)
	void PlayDeathMontage(const FGameplayEventData* TriggerEventData);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|Death")
	bool bAutoStartDeath;
};
