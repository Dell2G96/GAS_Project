// 26.11.12 신규 - AOE 장판 안에서 받는 밀기/당기기 힘 (피해자 쪽 어빌리티)

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_ZoneForce.generated.h"

/**
 * AOE 장판 힘 어빌리티 (피해자 쪽, GameplayEvent 트리거형).
 * AOE 예고 액터가 보낸 Souls.Events.Combat.ZoneForce 이벤트를 받아, 장판 중심 기준 원형 루트모션 힘(RadialForce)을 건다.
 * 힘은 장판 반경 안에 있을 때만 작용하고, 가산(Additive) 방식이라 플레이어 입력과 합쳐진다 → 버티며 움직일 수 있다.
 * ServerInitiated라 서버와 소유 클라이언트가 같이 적용 → 멀티에서 끊김 없음 (넉백 어빌리티와 같은 방식).
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_ZoneForce : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_ZoneForce(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * [서버] 피해자에게 장판 힘 이벤트를 보내는 공용 함수.
	 * Strength(cm/s): 양수 = 중심에서 밀어냄, 음수 = 중심으로 당김. Duration: 힘이 유지되는 시간(초)
	 * 사망 중·슈퍼아머 중이면 보내지 않는다 (넉백과 같은 규칙).
	 */
	static void SendZoneForceEvent(AActor* InstigatorActor, AActor* VictimActor, const FVector& Center, float Radius, float Strength, float Duration);

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 힘 유지 시간이 끝나면 어빌리티 종료
	UFUNCTION()
	void OnZoneForceFinished();
};
