#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LeeGameplayAbility_Knockback.h"
#include "LeeHitOverride.generated.h"

/**
 * 타격 하나에만 적용할 데미지·넉백 덮어쓰기 값.
 * 다단 공격 몽타주에서 판정 노티파이(WeaponTrace/ConeTrace)마다 다른 값을 주고 싶을 때 쓴다.
 * 체크하지 않은 항목은 공격 GA의 AttackData 값을 그대로 쓴다.
 */
USTRUCT(BlueprintType)
struct FLeeHitOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitOverride", meta = (InlineEditConditionToggle))
	bool bOverrideDamage = false;

	/** 이 타격의 데미지 (AttackData.BaseDamage 대신 사용하는 절대값) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitOverride", meta = (EditCondition = "bOverrideDamage", ClampMin = "0.0"))
	float BaseDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitOverride", meta = (InlineEditConditionToggle))
	bool bOverrideKnockback = false;

	/** 이 타격의 넉백 (AttackData.Knockback 대신 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitOverride", meta = (EditCondition = "bOverrideKnockback"))
	FLeeKnockbackParams Knockback;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class ULeeHitOverrideProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 히트 이벤트를 보내는 판정 노티파이가 구현하는 인터페이스.
 * 노티파이는 히트 이벤트 Payload.OptionalObject에 자신을 싣고, 공격 GA는 이 인터페이스로만 값을 읽는다
 * (GA가 특정 노티파이 클래스에 의존하지 않게 하기 위함).
 */
class GAS_PROJECT_API ILeeHitOverrideProvider
{
	GENERATED_BODY()

public:
	virtual const FLeeHitOverride& GetHitOverride() const = 0;
};
