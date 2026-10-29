// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "LeeProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;

/**
 * Enemy 원거리 공격이 발사하는 단순 투사체.
 * 피해 판정은 직접 하지 않는다 — GA가 만든 GameplayEffectSpec을 그대로 들고 다니다가
 * 맞은 대상의 ASC에 ApplyGameplayEffectSpecToSelf로 넘겨, 기존 ULeeExecCalc_Damage
 * 판정 경로(가드/퍼펙트가드/회피 무적)를 근접 공격과 동일하게 통과하게 한다.
 */
UCLASS()
class GAS_PROJECT_API ALeeProjectile : public AActor
{
	GENERATED_BODY()

public:
	ALeeProjectile();

	/** GA가 발사 직전에 호출 — 데미지 Spec과 발사자를 주입한다 */
	void InitProjectile(const FGameplayEffectSpecHandle& InDamageSpec, AActor* InInstigatorActor);

	/** GA가 발사 직전에 호출 — 명중 시 넉백 값(비행 방향 기준 오프셋/시간/가드 배율)을 주입한다 */
	void InitKnockback(const FVector& InOffset, float InDuration, float InGuardScale);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Lee|Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Lee|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** [테스트용] 2026.10.20 — 투사체를 눈으로 확인하기 위한 기본 구체 메시. 실제 무기 메시가 준비되면 교체한다 */
	UPROPERTY(VisibleAnywhere, Category = "Lee|Projectile")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	/** [넉백][서버] 비행 방향 기준 오프셋을 월드 오프셋으로 바꿔 피해자에게 넉백 이벤트를 보낸다 */
	void SendKnockbackEvent(AActor* HitActor, const UAbilitySystemComponent* HitASC, bool bPerfectGuard) const;

private:
	FGameplayEffectSpecHandle DamageSpec;

	FVector KnockbackOffset = FVector::ZeroVector;
	float KnockbackDuration = 0.2f;
	float GuardKnockbackScale = 0.5f;

	UPROPERTY()
	TObjectPtr<AActor> InstigatorActorRef;
};
