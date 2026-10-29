
#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_Knockback.generated.h"

/**
 * 넉백 어빌리티 (피해자 쪽, GameplayEvent 트리거형)
 * 공격 어빌리티가 보낸 Souls.Events.Combat.Knockback 이벤트의 월드 오프셋/시간으로
 * 루트모션(MoveToForce)을 적용해 피해자를 민다. ServerInitiated라 서버와 소유 클라이언트가 같이 적용 → 멀티에서 끊김 없음
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_Knockback : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_Knockback(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 루트모션 이동이 끝나면 어빌리티 종료
	UFUNCTION()
	void OnKnockbackFinished();
};
