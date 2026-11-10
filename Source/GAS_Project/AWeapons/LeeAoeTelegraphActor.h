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

/**
 * AOE 예고 액터 (리플리케이트).
 * 바닥에 반경 Radius의 원(데칼)을 띄우고 TelegraphTime 동안 안쪽을 채운다(머티리얼 파라미터 Progress 0→1).
 * 시간이 다 되면 서버가 원 안의 적에게 한 번씩 데미지 Spec + 바깥 방향 넉백을 주고, 모두의 화면에 폭발 이펙트를 낸다.
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

	/** 바닥 예고 원 */
	UPROPERTY(VisibleAnywhere, Category = "Lee|Aoe")
	TObjectPtr<UDecalComponent> TelegraphDecal;

	/** 폭발할 때 낼 나이아가라 (비우면 이펙트 없음) */
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Aoe")
	TObjectPtr<UNiagaraSystem> ExplosionEffect;

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
	/** [서버] 원 안의 적에게 데미지·넉백 */
	void Explode();

	/** [모두] 폭발 이펙트 + 예고 원 끄기 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayExplosion();

	FGameplayEffectSpecHandle DamageSpec;

	FLeeKnockbackParams Knockback;

	TWeakObjectPtr<AActor> InstigatorActorRef;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DecalMaterial;

	/** 예고가 시작된 뒤 지난 시간 (각 화면이 자기 시계로 센다) */
	float ElapsedTime = 0.0f;

	bool bExploded = false;
};
