// 26.11.10 신규 - Enemy AOE 공격의 바닥 예고 원 + 폭발 판정 액터

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "GAS_Project/AAbilitySystem/Abilities/LeeGameplayAbility_Knockback.h"
#include "LeeAoeTelegraphActor.generated.h"

class UDecalComponent;
class UMaterialInstanceDynamic;
class UNiagaraSystem;
class UNiagaraComponent;
class USphereComponent;

/**
 * AOE 예고 액터 (리플리케이트).
 * 바닥에 반경 Radius의 원(데칼)을 띄우고 TelegraphTime 동안 안쪽을 채운다(머티리얼 파라미터 Progress 0→1).
 * 시간이 다 되면 모두의 화면에 폭발 이펙트를 내고, 서버는 충돌 구(HitSphere)를 중심에서 Radius까지 키우며
 * 처음 닿은 적에게 한 번씩 데미지 Spec + 바깥 방향 넉백을 준다 (26.11.12 수정 - 이펙트가 퍼지는 모양에 맞춘 판정).
 */
UCLASS()
class GAS_PROJECT_API ALeeAoeTelegraphActor : public AActor
{
	GENERATED_BODY()

public:
	ALeeAoeTelegraphActor();

	/** [서버] 스폰 직후(FinishSpawning 전)에 호출 — 데미지 Spec, 공격자, 범위, 예고 시간, 넉백을 넣는다 */
	void InitAoe(const FGameplayEffectSpecHandle& InDamageSpec, AActor* InInstigatorActor, float InRadius, float InTelegraphTime,
		const FLeeKnockbackParams& InKnockback);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** 26.11.12 추가 - 액터가 사라질 때 폭발 이펙트를 멈춘다 (반복 재생 이펙트가 계속 남는 문제) */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 26.11.12 추가 - 폭발 이펙트를 띄울 위치 오프셋(cm) — 바닥에 딱 붙은 평면 이펙트가 바닥에 가려지지 않게 살짝 띄운다 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Aoe")
	FVector ExplosionEffectOffset = FVector(0.0f, 0.0f, 5.0f);

	/** 바닥 예고 원 */
	UPROPERTY(VisibleAnywhere, Category = "Lee|Aoe")
	TObjectPtr<UDecalComponent> TelegraphDecal;

	/** 폭발할 때 낼 나이아가라 (비우면 이펙트 없음) */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Aoe")
	TObjectPtr<UNiagaraSystem> ExplosionEffect;

	/** 26.11.12 추가 - 폭발 판정용 충돌 구 (Pawn과 Overlap만, 서버에서 폭발 후에만 켠다) */
	UPROPERTY(VisibleAnywhere, Category = "Lee|Aoe|Hit")
	TObjectPtr<USphereComponent> HitSphere;

	/** 26.11.12 추가 - 폭발 후 충돌 구가 커지기 시작할 때까지의 시간(초) — 이펙트가 땅에서 솟는 시점에 맞춘다 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Aoe|Hit", meta = (ClampMin = "0.0", ForceUnits = "s"))
	float HitStartDelay = 0.0f;

	/** 26.11.12 추가 - 충돌 구가 0에서 Radius까지 커지는 시간(초) — 이펙트가 퍼지는 속도에 맞춘다. 0이면 한 번에 전체 범위 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Aoe|Hit", meta = (ClampMin = "0.0", ForceUnits = "s"))
	float HitExpandTime = 0.3f;

	/** 26.11.12 추가 - 폭발 후 장판이 남아 있는 시간(초). 0이면 장판 없음(폭발 판정만). 이펙트도 이 시간 동안 유지된다 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Aoe|Zone", meta = (ClampMin = "0.0", ForceUnits = "s"))
	float ZoneDuration = 0.0f;

	/** 26.11.12 추가 - 장판 안의 적을 미는 힘(cm/s). 양수 = 중심에서 밀어냄, 음수 = 중심으로 당김, 0 = 힘 없음 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Aoe|Zone")
	float ZoneForceStrength = 0.0f;

	/** 26.11.12 추가 - 장판 틱 간격(초) — 이 간격마다 장판 안의 적을 찾아 틱 데미지를 주고, 새로 들어온 적에게 힘을 건다 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Aoe|Zone", meta = (ClampMin = "0.05", ForceUnits = "s"))
	float ZoneTickInterval = 0.5f;

	/** 26.11.12 추가 - 틱마다 주는 데미지 (양수, 0이면 틱 데미지 없음). 폭발 데미지 Spec을 복사해 크기만 바꿔 쓴다 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Aoe|Zone", meta = (ClampMin = "0.0"))
	float ZoneTickDamage = 0.0f;

	/** 폭발 이펙트 크기 = Radius / 이 값 (이펙트가 반경 1m 기준으로 만들어졌으면 100) */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Aoe", meta = (ClampMin = "1.0"))
	float ExplosionEffectBaseRadius = 100.0f;

	/** 데칼 머티리얼의 채움 정도 파라미터 이름 (0 = 테두리만, 1 = 가득 참) */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Aoe")
	FName ProgressParameterName = TEXT("Progress");

	/** 데칼이 위아래로 투영되는 깊이(cm, 반쪽 크기) — 경사·계단에서도 원이 보이게 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Aoe", meta = (ClampMin = "10.0"))
	float DecalHalfDepth = 150.0f;

	/** 폭발 후 액터가 사라지기까지의 시간(초) — 이펙트가 끝까지 재생되도록 */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Aoe", meta = (ClampMin = "0.1"))
	float LingerTimeAfterExplosion = 1.5f;

	/** 폭발 순간 BP에서 소리 등을 추가로 낼 때 쓰는 이벤트 (모든 화면에서 호출) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lee|Aoe")
	void BP_OnExploded();

	/** 범위 반경(cm) — 스폰 때 한 번만 복제 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lee|Aoe")
	float Radius = 300.0f;

	/** 예고 시간(초) — 스폰 때 한 번만 복제 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Lee|Aoe")
	float TelegraphTime = 1.0f;

private:
	/** [서버] 폭발 시작 — 이펙트를 내고 충돌 구 판정을 시작한다 */
	void Explode();

	/** 26.11.12 추가 - [서버] 폭발 후 매 프레임 충돌 구를 키운다. 다 커지면 판정을 끝낸다 */
	void UpdateHitSphere(float DeltaSeconds);

	/** 26.11.12 추가 - [서버] 충돌 구가 Pawn에 닿음 */
	UFUNCTION()
	void OnHitSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 26.11.12 추가 - [서버] 한 대상에게 한 번만: 아군 제외 → 데미지 Spec → (퍼펙트 가드가 아니면) 바깥 방향 넉백 */
	void ApplyHit(AActor* Victim);

	/** 26.11.12 추가 - [서버] 공격자·아군을 빼고 데미지를 줄 수 있는 대상인지 */
	bool CanAffect(AActor* Victim) const;

	/** 26.11.12 추가 - [서버] 장판 유지 중 매 프레임: 틱 간격마다 장판 안의 적에게 틱 데미지, 처음 들어온 적에게 밀기/당기기 힘 */
	void UpdateZone(float DeltaSeconds);

	/** 26.11.12 추가 - [서버] 장판 틱 1회 */
	void ZoneTick();

	/** [모두] 폭발 이펙트 + 예고 원 끄기 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayExplosion();

	FGameplayEffectSpecHandle DamageSpec;

	FLeeKnockbackParams Knockback;

	TWeakObjectPtr<AActor> InstigatorActorRef;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DecalMaterial;

	/** 26.11.12 추가 - 이 화면에서 띄운 폭발 이펙트 (EndPlay에서 멈추기 위해 보관) */
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ExplosionComponent;

	/** 예고가 시작된 뒤 지난 시간 (각 화면이 자기 시계로 센다) */
	float ElapsedTime = 0.0f;

	bool bExploded = false;

	/** 26.11.12 추가 - [서버] 충돌 구 판정 중인지 / 폭발 후 지난 시간 / 이미 맞은 대상 */
	bool bHitActive = false;
	float HitElapsedTime = 0.0f;
	TSet<TWeakObjectPtr<AActor>> HitActors;

	/** 26.11.12 추가 - [서버] 장판 유지 중인지 / 폭발 후 지난 시간 / 다음 틱까지 남은 시간 / 이미 힘을 건 대상 */
	bool bZoneActive = false;
	float ZoneElapsedTime = 0.0f;
	float ZoneTickRemaining = 0.0f;
	TSet<TWeakObjectPtr<AActor>> ForcedActors;
};
