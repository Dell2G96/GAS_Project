// 26.11.08 신규 - 마법 쉴드가 깨질 때 유리 조각이 흩어지는 연출 액터

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LeeShieldShatterActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

/** 유리 조각 하나의 움직임 정보 */
USTRUCT()
struct FLeeShieldShard
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Mesh;

	FVector Velocity = FVector::ZeroVector;
	FRotator SpinPerSecond = FRotator::ZeroRotator;
	FVector StartScale = FVector::OneVector;
};

/**
 * 쉴드 구 표면에 유리 조각 메시를 고르게 깔아 두고, 바깥으로 튀며 떨어지다 작아져 사라지게 한다.
 * 쉴드 Cue(ALeeGameplayCue_MagicShield)가 깨질 때 스폰하며, 쉴드 Cue가 지워져도 혼자 남아 연출을 끝낸다.
 */
UCLASS()
class GAS_PROJECT_API ALeeShieldShatterActor : public AActor
{
	GENERATED_BODY()

public:
	ALeeShieldShatterActor();

	/** 조각 생성과 함께 연출 시작. PushDirection = 조각이 더 밀려 나갈 방향(공격 반대쪽) */
	void StartShatter(UStaticMesh* ShardMesh, UMaterialInterface* ShardMaterial, float Radius,
		int32 ShardCount, float Lifetime, float Speed, const FVector& PushDirection);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(Transient)
	TArray<FLeeShieldShard> Shards;

	float Elapsed = 0.0f;
	float TotalLifetime = 1.0f;
};
