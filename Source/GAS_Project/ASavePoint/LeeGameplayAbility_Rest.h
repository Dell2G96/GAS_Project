// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/AAbilitySystem/Abilities/LeeGameplayAbility.h"
#include "LeeGameplayAbility_Rest.generated.h"

class UGameplayEffect;

// 세이브 포인트 휴식 어빌리티 (서버 전용, Ability.Interaction.Activate 이벤트로 트리거)
// HP/스태미나 회복(GE) + 상태 GE 제거 + 마지막 휴식 세이브 포인트 기록 + Enemy 리셋. Enemy가 이 플레이어를 타겟 중이면 활성화 불가
UCLASS(Abstract)
class GAS_PROJECT_API ULeeGameplayAbility_Rest : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_Rest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 휴식 시 적용할 회복 GE (Health/Stamina를 Max로 Override)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Rest")
	TSubclassOf<UGameplayEffect> RestEffect;

	// 이 태그를 부여하는 활성 GE를 휴식 시 제거 (예: 그로기, 스태미나 재생 지연)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Rest")
	FGameplayTagContainer EffectTagsToRemoveOnRest;

private:
	// 살아있는 Enemy 중 하나라도 이 Pawn을 현재 타겟으로 잡고 있으면 true (서버에서만 유효)
	static bool IsTargetedByEnemy(const AActor* PlayerPawn);
};
