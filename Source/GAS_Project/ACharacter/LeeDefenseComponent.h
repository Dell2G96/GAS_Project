#pragma once
 
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LeeDefenseComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

/**
 * 플레이어/Enemy 공통 방어 후처리 컴포넌트 (서버 전용 로직).
 * LeeCharacter 계열 BP(Player, Enemy)에 공통으로 부착한다 (Q1: 공통 컴포넌트 방식 결정 반영).
 *
 * 역할:
 *  1) LeeSoulsStatSet::OnDamageResolved 수신 → 판정 결과 태그(Souls.DamageResult.*)별로
 *     GameplayEvent 발송 + 반대편 GE 적용 (ExecCalc는 계산만 하므로 여기서 부수효과 담당)
 *  2) LeeSoulsStatSet::OnOutOfStamina 수신 → 원인 태그로 GuardBreak/PostureBreak 분기 
 *     + GE_Groggy 적용 (Enemy는 기존 LeeFinisherTargetComponent와 중복 적용되지 않도록 태그 검사)
 *  3) 가드 수치(GuardStaminaCost/GuardAngleDeg) 보관 — ULeeExecCalc_Damage가 읽어간다 
 */

UCLASS(Blueprintable, ClassGroup = (Lee), meta = (BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeDefenseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeeDefenseComponent();

	float GetGuardStaminaCost() const { return GuardStaminaCost; }
	float GetStaminaDamageOnParry() const { return StaminaDamageOnParry; }
	float GetGuardAngleDeg() const { return GuardAngleDeg; }

private:
	/** 오너 ASC의 SoulsStatSet 델리게이트 바인딩 (ASC 초기화 순서 때문에 다음 틱에 시도) */
	void BindToStatSetDelegates();

	/** [서버] 데미지 판정 확정 수신 → 결과 태그별 이벤트 발송/GE 적용 */
	void HandleDamageResolved(AActor* EffectInstigator, AActor* EffectCauser,
		const struct FGameplayEffectSpec* EffectSpec, float EffectMagnitude, float OldValue, float NewValue);

	/** [서버] 스태미나 0 도달 → 원인 태그로 GuardBreak/PostureBreak 분기 + 그로기 적용 */
	void HandleOutOfStamina(AActor* EffectInstigator, AActor* EffectCauser,
		const struct FGameplayEffectSpec* EffectSpec, float EffectMagnitude, float OldValue, float NewValue);

	/** 그로기 GE 적용 (이미 그로기면 중복 적용하지 않음 — FinisherTargetComponent와 공존 안전) */
	void ApplyGroggy();

	/** Exclusive 그룹(공격 등) 어빌리티 취소 — HitReaction이 활성화될 슬롯 확보  */
	static void CancelExclusiveAbilities(UAbilitySystemComponent* ASC);

	/** GameplayEvent 발송 헬퍼 (Instigator = 상대편 액터) */
	static void SendGameplayEventTo(AActor* TargetActor, const FGameplayTag& EventTag, AActor* InstigatorActor);

	UAbilitySystemComponent* GetOwnerASC() const;
	
	
	
protected:
	virtual void BeginPlay() override;

	/** 일반 가드 피격 1회당 방어자 스태미나 소모량 (ExecCalc가 읽음) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Defense", meta = (ClampMin = "0.0"))
	float GuardStaminaCost = 20.0f;

	/** 퍼펙트 가드(패리) 성공 시 공격자 스태미나 감소량 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Defense", meta = (ClampMin = "0.0"))
	float StaminaDamageOnParry = 30.0f;

	/** 가드가 성립하는 전방 원뿔 전체 각 (도). 밖에서 맞으면 일반 피격 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Defense", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float GuardAngleDeg = 180.0f;

	/** [서버] 가드/패리 브레이크 시 적용할 그로기 GE */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Defense|Effect")
	TSubclassOf<UGameplayEffect> GroggyEffect;

	/** [서버] 패리 시 공격자에게 적용할 스태미나 감소 GE. 
	 * BP에서 GE_StaminaDamage 지정 (SetByCaller: Souls.SetByCaller.StaminaDamage) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Defense|Effect")
	TSubclassOf<UGameplayEffect> StaminaDamageEffect;

	/** [서버] 가드 피격 시 잠시 스태미나 회복을 차단하는 Duration GE
	 * BP에서 GE_RegenBlockOnHit 지정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Defense|Effect")
	TSubclassOf<UGameplayEffect> RegenBlockOnHitEffect;

};
