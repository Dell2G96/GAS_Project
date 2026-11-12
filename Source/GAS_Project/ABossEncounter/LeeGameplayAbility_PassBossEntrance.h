// 26.11.12 신규 - 안개벽(봉쇄된 입구) 통과 어빌리티

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/AAbilitySystem/Abilities/LeeGameplayAbility.h"
#include "LeeGameplayAbility_PassBossEntrance.generated.h"

// 보스 입구에서 F를 누르면 실행되는 서버 전용 어빌리티 (Ability.Interaction.Activate 이벤트로 트리거)
// 입구 액터의 PassThrough를 호출해 아레나 안쪽으로 이동시킨다
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_PassBossEntrance : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_PassBossEntrance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
