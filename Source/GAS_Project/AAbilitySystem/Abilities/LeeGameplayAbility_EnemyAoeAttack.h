// 26.11.10 신규 - Enemy AOE(범위) 공격 어빌리티 (모르가나 자기 중심 원형 / 대상 발밑 장판)

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_Knockback.h"
#include "LeeGameplayAbility_EnemyAoeAttack.generated.h"

class UAnimMontage;
class UGameplayEffect;
class ALeeAoeTelegraphActor;

/** AOE 원의 중심을 어디에 둘지 */
UENUM(BlueprintType)
enum class ELeeAoeCenterMode : uint8
{
	/** 시전자(모르가나) 발밑 — 붙어 있는 플레이어를 떼어낸다 */
	Self,
	/** AI가 바라보는 대상(Focus) 발밑 — 멀리 있는 플레이어를 움직이게 한다 */
	Target,
};

/**
 * Enemy AOE 공격 (서버 실행).
 * 중심 위치를 정해 데미지 Spec을 실은 예고 액터(ALeeAoeTelegraphActor)를 바닥에 깐다. 실제 판정은 예고 액터가 시간이 되면 한다.
 * 몽타주가 비어 있으면 바로 깔고 끝나고, 있으면 Fire 이벤트(없으면 몽타주 끝) 시점에 깐다.
 * 같은 C++ 클래스를 BP 두 개(CenterMode만 다름)로 나눠 쓴다. Ability Tag·쿨다운 GE는 BP에서 지정한다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_EnemyAoeAttack : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_EnemyAoeAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** 원의 중심 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Aoe")
	ELeeAoeCenterMode CenterMode = ELeeAoeCenterMode::Self;

	/** 깔 예고 액터 (B_AoeTelegraph) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Aoe")
	TSubclassOf<ALeeAoeTelegraphActor> TelegraphActorClass;

	/** 범위 반경(cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Aoe", meta = (ClampMin = "10.0", ForceUnits = "cm"))
	float Radius = 400.0f;

	/** 원이 깔린 뒤 터지기까지의 시간(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Aoe", meta = (ClampMin = "0.05", ForceUnits = "s"))
	float TelegraphTime = 1.0f;

	/** 데미지 (양수로 입력) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Aoe|Damage", meta = (ClampMin = "0.0"))
	float BaseDamage = 25.0f;

	/** 데미지 GE (SetByCaller Damage + ExecCalc_Damage 경로, 예: GE_Melee_Damage) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Aoe|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** 추가 공격 속성 태그 (예: 가드 불가). DamageType_Attack_Aoe는 자동으로 붙는다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Aoe|Damage")
	FGameplayTagContainer ExtraDamageTypeTags;

	/** 넉백 — 오프셋 X가 중심에서 바깥으로 밀리는 거리 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Aoe|Damage")
	FLeeKnockbackParams Knockback;

	/** (선택) 시전 몽타주. 비우면 바로 깐다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Aoe|Montage")
	TObjectPtr<UAnimMontage> Montage;

	/** 몽타주 안에서 원을 까는 시점을 알리는 이벤트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Aoe|Montage")
	FGameplayTag CastEventTag;

private:
	UFUNCTION()
	void OnCastEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	/** [서버] 중심 위치를 정해 예고 액터를 한 번만 깐다 */
	void SpawnTelegraph();

	/** 중심 위치 계산 (대상 모드인데 대상이 없으면 false) */
	bool FindCenterLocation(FVector& OutLocation) const;

	/** 이번 시전에서 이미 깔았는지 */
	bool bSpawned = false;
};
