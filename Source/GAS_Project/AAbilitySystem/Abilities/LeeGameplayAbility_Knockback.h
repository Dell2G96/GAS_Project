
#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_Knockback.generated.h"

/**
 * 넉백 값 묶음 (근접 AttackData, 원거리 AttackData, 투사체, 타격별 Override가 같이 사용).
 * 오프셋은 기준 방향(공격자 또는 투사체의 Yaw) 기준이다. X=앞(밀어내기), Y=오른쪽, Z=위.
 */
USTRUCT(BlueprintType)
struct FLeeKnockbackParams
{
	GENERATED_BODY()

	/** 피해자를 밀어낼 거리(cm). 예: (300,0,0) = 뒤로 3m, (0,0,150) = 위로 1.5m. 0이면 넉백 없음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knockback")
	FVector Offset = FVector::ZeroVector;

	/** 위 거리를 이동하는 데 걸리는 시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knockback", meta = (ClampMin = "0.01"))
	float Duration = 0.2f;

	/** 피해자가 가드 중일 때 거리 배율 (0이면 가드 시 넉백 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knockback", meta = (ClampMin = "0.0"))
	float GuardScale = 0.5f;
};

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

	/**
	 * [서버] 넉백 이벤트를 피해자에게 보내는 공용 함수 (근접·투사체 공통 규칙).
	 * 값이 0이거나, 피해자가 사망 중이거나, 가드 배율이 0이면 보내지 않는다.
	 * DirectionYaw: 오프셋을 돌릴 기준 방향 (근접 = 공격자 Yaw, 투사체 = 비행 방향 Yaw)
	 */
	static void SendKnockbackEvent(AActor* InstigatorActor, AActor* VictimActor, const FLeeKnockbackParams& Params, float DirectionYaw);

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 루트모션 이동이 끝나면 어빌리티 종료
	UFUNCTION()
	void OnKnockbackFinished();
};
