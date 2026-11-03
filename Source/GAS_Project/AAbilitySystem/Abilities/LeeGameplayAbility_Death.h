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
	// 26.11.03 수정 - 재생한 몽타주 길이(초)를 반환 (몽타주가 없으면 0)
	float PlayDeathMontage(const FGameplayEventData* TriggerEventData);

	// 26.11.03 추가 - 몽타주 종료 + DeathHoldTime 후 호출: 모든 머신에 페이드 시작을 알리고 DeathFadeDuration 뒤 FinishDeath 예약 (서버)
	void StartDeathFade();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|Death")
	bool bAutoStartDeath;

	// 26.11.03 추가 - true면 사망 몽타주 → 대기 → 페이드 후 FinishDeath를 호출해 액터를 파괴한다 (적 Death 어빌리티용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|Death")
	bool bFinishDeathAfterMontage = false;

	// 26.11.03 추가 - 몽타주가 끝난 뒤 쓰러진 자세로 유지하는 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|Death", meta=(EditCondition="bFinishDeathAfterMontage", ClampMin="0.0"))
	float DeathHoldTime = 1.0f;

	// 26.11.03 추가 - 디졸브 페이드 시간(초). 이 시간이 지나면 FinishDeath → 액터 파괴
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|Death", meta=(EditCondition="bFinishDeathAfterMontage", ClampMin="0.0"))
	float DeathFadeDuration = 2.0f;

	// 26.11.03 추가 - 페이드 시작 / FinishDeath 예약용 타이머
	FTimerHandle DeathSequenceTimerHandle;
};
