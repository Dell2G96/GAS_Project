// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "LeeGameplayAbility_EnemyRangedAttack.generated.h"

class UAnimMontage;
class UGameplayEffect;
class ALeeProjectile;

/**
 * 원거리 공격 1개 엔트리 — 몽타주 + 투사체 + 데미지 + 공격 속성 태그.
 * FLeeMeleeAttackData와 짝을 맞춘 구조체 (근접은 트레이스, 원거리는 투사체 스폰).
 */
USTRUCT(BlueprintType)
struct FLeeRangedAttackData
{
	GENERATED_BODY()

	/** 재생할 시전 몽타주 (Fire 노티파이 포함) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimMontage> Montage = nullptr;

	/** 발사할 투사체 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ALeeProjectile> ProjectileClass;

	/** 명중 시 데미지. SetByCaller로 GE에 전달 (음수 변환은 적용 시점에 처리) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float BaseDamage = 20.0f;

	/** 데미지 GE Spec에 DynamicAssetTag로 실리는 속성 태그. Souls.DamageType.Attack.Projectile은 자동으로 추가된다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer DamageTypeTags;

	/** 발사 원점 소켓 이름. 메시에 없으면 액터 위치를 그대로 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MuzzleSocketName = TEXT("Muzzle");

	/** [넉백] 명중 시 피해자를 미는 거리(cm). 투사체 비행 방향 기준 X=앞, Y=오른쪽, Z=위. 0이면 넉백 없음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|Ranged|Knockback")
	FVector KnockbackOffset = FVector::ZeroVector;

	/** [넉백] 밀려나는 데 걸리는 시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|Ranged|Knockback", meta = (ClampMin = "0.01"))
	float KnockbackDuration = 0.2f;

	/** [넉백] 피해자가 일반 가드 중일 때 오프셋 배율 (퍼펙트 가드는 넉백 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|Ranged|Knockback", meta = (ClampMin = "0.0"))
	float GuardKnockbackScale = 0.5f;
};

/**
 * Enemy 원거리 공격 어빌리티.
 *
 * 기본 흐름:
 *  1) ActivateAbility -> 공격 데이터 검증 -> CommitAbility (비용/쿨다운은 BP에서 지정하지 않으면 무동작 —
 *     발사 빈도는 AttackTokenComponent/StateTree의 TimeSinceLastUse가 전담한다)
 *  2) ActivationOwnedTags에 의해 Souls.Status.Attack.Attacking 태그 자동 부여
 *     (Lee Activate Ability StateTree Task가 이 태그로 종료를 판정하는 fallback 경로이므로 필수)
 *  3) WaitGameplayEvent(FireEventTag) 등록 (다연발 몽타주 지원을 위해 반복 수신) -> PlayMontageAndWait
 *  4) Fire 이벤트 수신마다 그 시점 타겟 위치로 투사체 1개 발사 (발사 순간 현재 위치 조준, 선행 없음)
 *  5) 몽타주 완료/중단 -> EndAbility -> Attacking 태그 자동 제거
 *
 * AttackMelee와의 차이: 근접은 Trace 이벤트로 즉시 HitResult에 데미지를 적용하지만,
 * 원거리는 투사체를 날려 보내고 실제 피해 판정은 ALeeProjectile::OnProjectileHit에서 일어난다.
 * 두 경로 모두 최종적으로 같은 ULeeExecCalc_Damage를 거치므로 가드/퍼펙트가드/회피 무적 규칙이 동일하다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_EnemyRangedAttack : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_EnemyRangedAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
	/** 공격 데이터 풀. 1개 이상 등록. 활성화 시 랜덤 선택. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Ranged|Montage")
	TArray<FLeeRangedAttackData> AttackDataList;

	/** 명중 시 적용할 데미지 GE. ULeeExecCalc_Damage 기반 Execution을 가진 GE를 지정해야 가드/무적 규칙이 적용된다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Ranged|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** 몽타주 Fire 노티파이(Lee Gameplay Event (Server))가 발사하는 이벤트 태그 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Ranged|Event")
	FGameplayTag FireEventTag;

	/** [모션워핑] 시전 중 회전 정렬용 워프 타깃 이름 (AttackMelee와 동일 규약) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Ranged|Warp")
	FName WarpTargetName = TEXT("AttackWarp");

	/** 현재 활성화된 공격의 데이터 (활성화 시 확정, 종료 시 초기화) */
	UPROPERTY(Transient)
	FLeeRangedAttackData CurrentAttackData;

	/** 조준 기준 대상 — 기본(Enemy)은 AIController의 네이티브 Focus(AttackMelee와 동일 규약) */
	virtual AActor* GetAimTarget() const;

	/** 회전 전용 워프 타깃을 GetAimTarget() 기준으로 갱신한다 */
	void UpdateFacingWarpTarget();

	UFUNCTION()
	virtual void OnMontageCompleted();

	UFUNCTION()
	virtual void OnMontageInterrupted();

	/** Fire 이벤트 수신 — 그 시점 타겟 위치로 투사체 1개를 발사한다 */
	UFUNCTION()
	void OnFireEventReceived(FGameplayEventData Payload);

	/**
	 * 실제 발사 처리 (투사체 스폰 + 데미지 Spec 적용). OnFireEventReceived와, 몽타주가 없을 때의
	 * 즉시 발사 경로가 공유한다.
	 * [임시/테스트용] 2026.10.19 — 프로젝트에 Morgana용 캐스팅 몽타주가 아직 없어서,
	 * AttackDataList의 Montage가 비어있으면 애니메이션 없이 즉시 발사하도록 허용했다.
	 * 몽타주가 준비되면 이 분기는 제거하고 항상 Fire 이벤트를 거치도록 되돌려야 한다.
	 */
	void FireOnce();
};
