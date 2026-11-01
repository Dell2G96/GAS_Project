// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_Downed.generated.h"

// 26.11.02 추가 - 보스전 넉다운 어빌리티. GameplayEvent.Downed로 서버에서 시작된다.
// 임시 HP(최대 HP 비율) 설정 → N초 안에 동료가 부활시키면 HP 비율로 복귀, 못 하면 사망 (피격으로 HP 0이 돼도 사망)
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_Downed : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_Downed(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 부활 이벤트 수신 → HP 복구 후 종료
	UFUNCTION()
	void HandleRevived(FGameplayEventData Payload);

	// 넉다운 제한 시간 초과 → 사망
	void HandleDownedTimeout();

	// 넉다운 중 사망이 시작되면(피격으로 HP 0 / 전멸) 종료
	UFUNCTION()
	void HandleDeathStarted(AActor* OwningActor);

protected:
	// 넉다운 유지 시간(초). 이 시간 안에 부활하지 못하면 사망
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Downed", meta = (ClampMin = "0.1"))
	float DownedDuration = 50.0f;

	// 넉다운 진입 시 임시 HP (최대 HP 대비 비율)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Downed", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float DownedHealthRatio = 0.5f;

	// 부활 시 HP (최대 HP 대비 비율)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Downed", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float ReviveHealthRatio = 0.5f;

private:
	FTimerHandle DownedTimerHandle;
};
