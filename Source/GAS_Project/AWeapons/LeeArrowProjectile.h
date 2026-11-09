// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeProjectile.h"
#include "LeeArrowProjectile.generated.h"

class USkeletalMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 플레이어 활 화살. 서버가 손(소켓)에 붙인 채 스폰하고, 발사 때 떼어 날려 보낸다.
 * 명중 판정·데미지 적용은 부모 ALeeProjectile(서버 권위, ExecCalc_Damage 경로)을 그대로 쓴다.
 */
UCLASS()
class GAS_PROJECT_API ALeeArrowProjectile : public ALeeProjectile
{
	GENERATED_BODY()

public:
	ALeeArrowProjectile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** [서버] 손에서 떼어 Direction 방향으로 Speed(cm/s) 속도로 날리고, Range(cm)를 날면 사라지게 한다 */
	void Launch(const FVector& Direction, float Speed, float Range);

	bool IsLaunched() const { return bLaunched; }

protected:
	virtual void Destroyed() override;

	/** 화살 메시 (BP에서 메시 지정) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|Arrow")
	TObjectPtr<USkeletalMeshComponent> ArrowMesh;

	/** 날아가는 동안 켜지는 꼬리 이펙트 (BP에서 에셋 지정, 발사 때 켬) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lee|Arrow")
	TObjectPtr<UNiagaraComponent> TrailFX;

	/** 명중(또는 사거리 끝)에서 사라질 때 그 자리에 터뜨리는 이펙트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Arrow")
	TObjectPtr<UNiagaraSystem> ImpactFX;

	/** 화살 중력 배율 (0 = 직선 비행) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Arrow", meta = (ClampMin = "0.0"))
	float ArrowGravityScale = 0.0f;

private:
	/** 발사됐는지 — 클라에서는 꼬리 이펙트를 켜고, 사라질 때 명중 이펙트를 낼지 판단하는 데 쓴다 */
	UPROPERTY(ReplicatedUsing = OnRep_Launched)
	bool bLaunched = false;

	UFUNCTION()
	void OnRep_Launched();
};
