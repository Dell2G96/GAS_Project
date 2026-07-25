// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "LeeGameplayAbility_AttackMelee.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * 근접 공격 1개 엔트리 — 몽타주 + 이 몽타주 전용 데미지 + 공격 속성 태그.
 * 약공격/강공격은 이 구조체의 배열을 서로 다르게 채운 BP로 구분한다 (C++ 분기 없음).
 */
USTRUCT(BlueprintType)
struct FLeeMeleeAttackData
{
	GENERATED_BODY()

	/** 재생할 공격 몽타주 (단발 또는 다단히트 콤보 몽타주) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Montage = nullptr;

	/** 트레이스 이벤트 1회당 데미지. SetByCaller로 GE에 전달 (음수 변환은 적용 시점에 처리) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float BaseDamage = 10.0f;

	/**
	 * 이 몽타주에만 적용되는 공격 속성 태그.
	 * 데미지 GE Spec에 DynamicAssetTag로 실려 하위 시스템(강공격 피격 리액션, 가드 불가 판정 등)이 읽는다.
	 * 예: Souls.DamageType.Attack.Heavy, Souls.DamageType.Attack.Unblockable
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer DamageTypeTags;

	/**
	 * 공격 단계별 스태미나 비용 배열. 인덱스 0 = 1타, 1 = 2타, ...
	 * 몽타주에 배치된 Attack.CommitStep Notify 개수와 일치해야 한다.
	 * 예: 단발 → {15.0}, 3연타 → {5.0, 10.0, 20.0}
	 * 비어있으면 해당 몽타주는 스태미나 비용 없이 실행된다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
	TArray<float> StaminaCostPerStep;

	/** [모션워핑 하이브리드] 이 몽타주 재생 시 위치까지 보정할지(true) 회전만 보정할지(false) 콤보 타마다 설정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Warp")
	bool bWarpTranslation = false;

	/** bWarpTranslation=true일 때, 적으로부터 유지할 접근 거리(cm) — 대략 무기 리치 + 캡슐 반경 합 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Warp", meta = (EditCondition = "bWarpTranslation", ClampMin = "0.0"))
	float ApproachDistance = 150.0f;

	/** bWarpTranslation=true일 때, 이 거리보다 적이 멀면 위치 워프를 하지 않음(순간이동 방지). 0이면 제한 없음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Warp", meta = (EditCondition = "bWarpTranslation", ClampMin = "0.0"))
	float MaxWarpDistance = 400.0f;
};

/**
 * Enemy 근접 공격 어빌리티 (신규, Lyra Clone 스타일).
 *
 * 기본 흐름:
 *  1) ActivateAbility -> 공격 데이터 검증 -> CommitAbilityCooldown (Cost는 단계 이벤트에서 처리)
 *  2) ActivationOwnedTags에 의해 Souls.Status.Attack.Attacking 태그 자동 부여
 *  3) WaitGameplayEvent(Attack.CommitStep) 먼저 등록 (공격 단계 스태미나 비용용)
 *  4) WaitGameplayEvent(Trace) 등록, AttackDataList 중 1개 랜덤 선택 -> PlayMontageAndWait
 *  5) Attack.CommitStep 이벤트 수신 시 스태미나 비용 검사/차감 (부족하면 즉시 EndAbility)
 *  6) ANS_ToggleTrace가 발사하는 트레이스 이벤트 수신 시 HitResult로 SetByCaller 방식 데미지 GE 적용
 *  7) 몽타주 완료/중단 -> EndAbility -> Attacking 태그 자동 제거
 *
 * 스태미나 비용 처리:
 *  - 기존 CommitAbility(Cost+Cooldown 동시) 대신 CommitAbilityCooldown()만 사용
 *  - 공격 단계 시작 시 몽타주 Notify(Attack.CommitStep)가 이벤트를 발사
 *  - OnAttackStepEventReceived()가 GE_AttackStepStaminaCost를 단계당 1회 적용
 *  - 빗나가도 차감, 여러 대상을 맞혀도 단계당 1회만 차감 (데미지 이벤트와 완전 분리)
 *
 * 약공격/강공격 분리:
 *  - 이 C++ 클래스를 부모로 하는 BP 2개(예: GA_EnemyAttack_Light/Heavy)로 나눈다.
 *  - 각 BP가 AbilityTags에 고유 식별 태그(Souls.Abilities.Attack.Melee.Light/Heavy)를 추가하고,
 *    AttackDataList에 자신의 몽타주+데미지+속성 태그+StaminaCostPerStep을 채운다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_AttackMelee : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_AttackMelee(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
	/** 공격 데이터 풀 (몽타주+데미지+속성 태그). 1개 이상 등록. 활성화 시 랜덤 선택. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Montage")
	TArray<FLeeMeleeAttackData> AttackDataList;

	/** 히트 시 적용할 데미지 GE (SetByCaller 방식). BP에서 GE_MeleeDamage 지정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/**
	 * 공격 단계(스윙) 시작마다 공격자 자신에게 적용할 스태미나 차감 GE.
	 * SetByCaller 방식으로 단계별 비용을 전달한다.
	 * BP에서 GE_AttackStepStaminaCost를 지정.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Stamina")
	TSubclassOf<UGameplayEffect> AttackStepStaminaCostEffect;

	/**
	 * 공격 종료 후 스태미나 회복을 지연시키는 GE. BP에서 GE_StaminaRegenDelay 지정.
	 * Duration은 SetByCaller(Souls.SetByCaller.Duration) 방식이어야 하며,
	 * 적용 시 Souls.Status.Stamina.RegenBlocked 태그를 부여해 GE_StaminaRegen을 억제한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Stamina")
	TSubclassOf<UGameplayEffect> StaminaRegenDelayEffect;

	/** 공격 종료 후 스태미나 회복 지연 시간(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Stamina", meta = (ClampMin = "0.0"))
	float StaminaRegenDelayDuration = 1.0f;

	/**
	 * ANS_ToggleTrace가 발사하는 Trace 이벤트 태그.
	 * 기본값: MyTags::Abilities::Enemy::Trace
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Event")
	FGameplayTag TraceEventTag;

	/**
	 * 공격 단계 시작을 알리는 비용 이벤트 태그.
	 * 기본값: MyTags::Souls::Event_Attack_CommitStep ("Souls.Events.Attack.CommitStep")
	 * 몽타주에 배치된 Lee Gameplay Event (Server) Notify의 EventTag와 일치해야 한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Event")
	FGameplayTag AttackStepEventTag;

	/** [모션워핑] 몽타주의 Motion Warping NotifyState(Skew Warp)와 매칭되는 워프 타깃 이름. 몽타주 노티파이 설정과 동일 문자열이어야 한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Warp")
	FName WarpTargetName = TEXT("AttackWarp");

	/**
	 * [모션워핑] Warp 타깃 재계산 이벤트 태그. 기본값: Souls.Events.Attack.RefreshWarpTarget
	 * 다단 공격 몽타주에서 2번째+ Warp 구간 직전에 Lee Gameplay Event(Server) 노티파이가 이 태그를 발사하면
	 * 그 시점의 대상 위치로 워프 타깃을 다시 계산한다(각 구간이 최신 위치를 향하게).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Warp")
	FGameplayTag RefreshWarpTargetEventTag;

protected:
	/**
	 * 공격 데이터 목록 검증 훅. 기본은 "비어있지 않은가"만 확인한다.
	 * Player 파생 클래스는 콤보 몽타주가 정확히 1개여야 하므로 이 함수를 override해 추가 검증한다.
	 */
	virtual bool ValidateAttackDataList() const;

	/**
	 * 이번 활성화에서 재생할 공격 데이터를 고르는 훅.
	 * 기본(Enemy)은 AttackDataList에서 랜덤 1개를 반환한다.
	 * Player 파생 클래스는 콤보 첫 타(인덱스 0)를 반환하도록 override한다.
	 * 반환 nullptr이면 ActivateAbility가 즉시 종료한다.
	 */
	virtual const FLeeMeleeAttackData* SelectAttackData();

	/**
	 * 스태미나 비용을 검사/차감하는 공통 처리.
	 * - CostIndex: CurrentAttackData.StaminaCostPerStep에서 읽을 비용 인덱스
	 * - DedupKey : 이미 차감했는지 판별하는 중복 방지 키 (같은 단계의 Notify 재실행 방지)
	 * 동작:
	 * - 비용 배열 범위 밖 / 이미 DedupKey로 차감됨 → false 반환 (상태 변경 없음)
	 * - 스태미나 부족 → EndAbility 호출 후 false 반환
	 * - 정상 차감 → CommittedAttackSteps에 DedupKey 기록 후 true 반환
	 * Enemy(기존)는 단일 몽타주라 CostIndex==DedupKey(자동 증가 인덱스)를 넘기고,
	 * Player(개별 몽타주 콤보)는 각 타 데이터의 [0] 비용을 쓰되(CostIndex=0) 콤보 인덱스를 DedupKey로 넘겨
	 * "타마다 서로 다른 몽타주"이면서도 중복 차감을 막는다.
	 */
	virtual bool TryCommitAttackStepCost(int32 CostIndex, int32 DedupKey);

	/**
	 * 공격 몽타주 재생 태스크 생성/델리게이트 연결.
	 * Player 파생 클래스는 이 함수를 override하여 콤보 입력/윈도우 Task를 몽타주 재생 직전에 등록한다.
	 */
	virtual void PlayAttackMontage(UAnimMontage* Montage, FName StartSection);

	/** 몽타주 정상 완료 / 블렌드아웃 */
	UFUNCTION()
	virtual void OnMontageCompleted();

	/** 몽타주 피격 취소 / 외부 취소 */
	UFUNCTION()
	virtual void OnMontageInterrupted();

	/** SetByCaller Duration 방식 GE 적용 헬퍼 (공격 종료 후 회복 지연 등). Duration이 SetByCaller(Souls.SetByCaller.Duration)여야 한다 */
	FActiveGameplayEffectHandle ApplyDurationEffect(TSubclassOf<UGameplayEffect> EffectClass, float Duration);

	/**
	 * [모션워핑] 워프가 바라볼 대상을 반환하는 훅.
	 * 기본(Enemy) = AIController의 네이티브 Focus(GetFocusActor). Enemy AIController(BP, StateTree 기반)의
	 * STT_SetFocus 태스크가 Perception으로 얻은 타깃을 SetFocus()로 반영해두므로 이 값을 그대로 읽으면 된다.
	 * Player 파생 클래스는 락온 대상을 반환하도록 override한다.
	 */
	virtual AActor* GetWarpFacingTarget() const;

	/**
	 * [모션워핑] GetWarpFacingTarget()으로 얻은 대상 기준으로 워프 타깃을 갱신한다.
	 * CurrentAttackData.bWarpTranslation에 따라 위치+회전(A)/회전만(B)로 분기한다.
	 * 대상이 없거나 MotionWarpingComponent가 없으면 기존 타깃을 제거한다(제자리 공격으로 안전 폴백).
	 * 각 타 몽타주 재생 직전(PlayMontageAndWait 생성 직전)에 호출한다.
	 */
	void UpdateAttackWarpTarget();

	/** 현재 활성화된 공격의 데이터 (활성화 시 확정, 종료 시 초기화) */
	UPROPERTY(Transient)
	FLeeMeleeAttackData CurrentAttackData;

	/** 이번 활성화에서 도달한 공격 단계 인덱스 (0부터 시작, EndAbility 시 초기화). Enemy 전용 자동 증가 카운터 */
	int32 CurrentAttackStepIndex = 0;

	/** 이미 비용을 적용한 단계 인덱스 — Section Loop/네트워크 보정 등으로 인한 중복 차감 방지 */
	TSet<int32> CommittedAttackSteps;

	/**
	 * 실제로 공격이 시작되었는지(쿨다운 커밋까지 성공했는지) 여부.
	 * false인 채로 EndAbility가 호출되면(데이터 검증 실패, 쿨다운 실패 등) 회복 지연 GE를 적용하지 않는다.
	 */
	bool bAttackActuallyStarted = false;

	/**
	 * Attack.CommitStep 이벤트를 수신하여 공격 단계별 스태미나 비용을 검사하고 차감한다.
	 * 스태미나가 부족하면 EndAbility로 몽타주를 즉시 중단한다.
	 * Player 파생 클래스는 이 함수를 override하여 Notify가 전달하는 명시적 StepIndex를 사용한다.
	 */
	UFUNCTION()
	virtual void OnAttackStepEventReceived(FGameplayEventData Payload);

	/**
	 * [모션워핑] RefreshWarpTarget 이벤트 수신 → 현재 대상 위치로 워프 타깃을 재계산한다.
	 * 다단 공격에서 두 번째+ Warp 구간이 몽타주 시작 때의 낡은 위치가 아니라 그 시점의 위치를 향하게 한다.
	 */
	UFUNCTION()
	virtual void OnRefreshWarpTargetEventReceived(FGameplayEventData Payload);

private:
	/** ANS_ToggleTrace로부터 HitResult 이벤트를 수신하여 데미지 GE 적용 */
	UFUNCTION()
	void OnTraceEventReceived(FGameplayEventData Payload);
};
