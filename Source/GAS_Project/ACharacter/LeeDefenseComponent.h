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
 *  3) 가드 수치(GuardStaminaCost/GuardValidAngleDeg) 보관 — ULeeExecCalc_Damage가 읽어간다
 */

UCLASS(Blueprintable, ClassGroup = (Lee), meta = (BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeDefenseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeeDefenseComponent();

	float GetGuardStaminaCost() const { return GuardStaminaCost; }
	float GetStaminaDamageOnParry() const { return StaminaDamageOnParry; }
	/** 가드 유효범위(half-angle, 정면 기준 좌우 각각 허용 각도)를 반환 */
	float GetGuardValidAngleDeg() const { return GuardValidAngleDeg; }

	/**
	 * [서버] 그로기 진입의 유일한 입구 — GE_Groggy 적용 + PostureBreak 이벤트 발송.
	 * 단, 가드브레이크/패리당함 연출 중이면 즉시 진입하지 않고 예약만 해둔다
	 * (연출 몽타주가 끝난 뒤 NotifyReactionFinished()가 실제로 진입시킨다).
	 * 이미 그로기 상태면 아무것도 하지 않는다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Lee|Defense")
	void EnterGroggy(AActor* Instigator);

	/**
	 * [서버] 선행 리액션 연출(가드브레이크/패리당함) 시작을 알린다.
	 * 이 구간에 스태미나가 고갈돼도 그로기 진입은 연출이 끝날 때까지 보류된다.
	 * 패리 경로처럼 "스태미나 감소 GE가 동기적으로 그로기를 유발하는" 상황을 막기 위해,
	 * GE를 적용하기 전에 먼저 호출해야 한다.
	 */
	void BeginReaction();

	/** [서버] 선행 리액션 몽타주 종료 — 보류돼 있던 그로기가 있으면 지금 진입시킨다 */
	void NotifyReactionFinished(AActor* Instigator);

	/** [디버그] 가드 판정 삼각형 시각화용 틱 — bDrawGuardArcDebug가 true일 때만 활성 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** [디버그] 가드 상태일 때 Player를 꼭짓점으로 하는 전방 삼각형(GuardValidAngleDeg)을 월드에 그린다 */
	void DrawGuardArcDebug() const;


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

	/**
	 * [서버] 가드 브레이크 처리 — 가드 어빌리티 강제 종료 + GuardBreak 이벤트 발송.
	 * 그로기는 여기서 걸지 않는다. 가드브레이크 몽타주가 끝난 뒤 NotifyReactionFinished()가 진입시킨다.
	 * "스태미나가 0으로 떨어지는 순간"(OnOutOfStamina)과 "이미 0인 채 가드 피격"(OnDamageResolved)
	 * 두 경로가 이 함수 하나로 수렴하며, bReactionInProgress로 이중 발송을 막는다.
	 */
	void BreakGuard(AActor* Attacker);

	/** [서버] 연출 어빌리티가 콜백을 주지 못한 경우의 안전장치 — 보류된 그로기를 강제로 진입시킨다 */
	void ForceEnterPendingGroggy();

	/** [서버] 선행 리액션(가드브레이크/패리당함) 몽타주 재생 중 여부. 이 구간에는 그로기 진입을 보류한다 */
	bool bReactionInProgress = false;

	/** [서버] 연출이 끝나면 그로기에 진입해야 하는지 (연출 중 스태미나가 고갈된 경우 세워짐) */
	bool bGroggyPending = false;

	/** [서버] 보류된 그로기를 넘겨줄 Instigator (몽타주 종료 시점까지 보관) */
	TWeakObjectPtr<AActor> PendingGroggyInstigator;

	/** [서버] 보류 그로기 안전장치 타이머 */
	FTimerHandle PendingGroggyTimeoutHandle;

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

	/**
	 * 가드 유효범위 — 정면(Forward) 기준 "좌우 각각" 몇 도까지 가드가 성립하는지(half-angle, 도).
	 * 예: 45로 설정하면 정면 기준 ±45도(총 90도) 범위 안에서 맞아야 가드 성립.
	 * 이 범위를 벗어나면(예: 완전 측면 90도, 후면 등) 가드 중이어도 일반 피격으로 처리된다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Defense", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float GuardValidAngleDeg = 45.0f;

	/** [디버그] 가드 중 전방 삼각형을 화면에 그릴지 여부. BP에서 켜고 끈다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Defense|Debug")
	bool bDrawGuardArcDebug = false;

	/** [디버그] 가드 삼각형 변의 길이(cm) — Player(꼭짓점)에서 좌/우 끝점까지 거리 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Defense|Debug", meta = (ClampMin = "10.0"))
	float GuardArcDebugRadius = 200.0f;

	/** [서버] 가드/패리 브레이크 시 적용할 그로기 GE */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Defense|Effect")
	TSubclassOf<UGameplayEffect> GroggyEffect;

	/**
	 * [서버] 선행 리액션 연출을 기다리는 그로기의 최대 보류 시간(초).
	 * 몽타주 미설정 등으로 GA_HitReaction이 종료 콜백을 주지 못해도 이 시간이 지나면 그로기에 진입한다.
	 * 가장 긴 가드브레이크/패리당함 몽타주보다 넉넉하게 잡을 것.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Defense|Effect", meta = (ClampMin = "0.1"))
	float PendingGroggyTimeout = 3.0f;

	/** [서버] 패리 시 공격자에게 적용할 스태미나 감소 GE. 
	 * BP에서 GE_StaminaDamage 지정 (SetByCaller: Souls.SetByCaller.StaminaDamage) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Defense|Effect")
	TSubclassOf<UGameplayEffect> StaminaDamageEffect;

	/** [서버] 가드 피격 시 잠시 스태미나 회복을 차단하는 Duration GE
	 * BP에서 GE_RegenBlockOnHit 지정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Defense|Effect")
	TSubclassOf<UGameplayEffect> RegenBlockOnHitEffect;

};
