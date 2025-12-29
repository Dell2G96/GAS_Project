#pragma once

#include "CoreMinimal.h"
#include "CGameplayAbility.h"
#include "GA_Roll.generated.h"

UCLASS()
class GAS_PROJECT_API UGA_Roll : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Roll();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	FVector ComputeRollDirection2D_Local(class ACCharacter* Character) const;
	// 오너(로컬) 입력/가속 기반으로 2D 롤 방향을 계산합니다.

	void PlayRollMontage();
	// GAS 태스크로 롤 몽타주를 재생합니다.

	UFUNCTION()
	void OnMontageEnded();
	// 몽타주 종료(완료/중단/블렌드아웃 포함) 시 어빌리티를 종료합니다.

	UFUNCTION()
	void OnFailsafeTimeout();
	// 몽타주 이벤트가 안 오는 상황을 대비한 안전 종료용 함수입니다.

private:
	void ResetTransientState();
	// InstancedPerActor에서 실행 간 상태가 남지 않도록 매 실행/종료 시 초기화합니다.

protected:
	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* RollMontage = nullptr;
	// 롤 몽타주입니다.

	UPROPERTY(EditDefaultsOnly)
	float RollDistance = 600.f;
	// 워프 타깃 거리입니다.

	UPROPERTY(EditDefaultsOnly)
	float FailsafeExtraTime = 0.15f;
	// 몽타주 길이 + 이 시간 후에도 종료가 안 되면 강제 종료합니다.

private:
	bool bMontageStarted = false;
	// 몽타주 중복 재생 방지 플래그입니다.

	TWeakObjectPtr<class ACPlayerCharacter> CachedCharacter;
	// 캐릭터 캐시입니다.
};
