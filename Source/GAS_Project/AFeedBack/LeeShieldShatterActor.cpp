// 26.11.08 신규 - 마법 쉴드가 깨질 때 유리 조각이 흩어지는 연출 액터

#include "LeeShieldShatterActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace LeeShieldShatter
{
	constexpr float Gravity = 1200.0f;        // 조각이 떨어지는 가속도 (cm/s²)
	constexpr float FadeStartRatio = 0.55f;   // 수명의 이 비율부터 작아지기 시작
	constexpr float ShardSizeAt110 = 2.2f;    // 반지름 110cm 쉴드일 때 조각 메시 배율 (원본 조각 약 30cm)
}

// 생성자 — 매 프레임 조각을 움직이기 위해 Tick 사용, 루트만 둔다
ALeeShieldShatterActor::ALeeShieldShatterActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

// 구 표면에 조각을 고르게(피보나치 구) 배치하고, 바깥 방향 속도·회전을 무작위로 준다
void ALeeShieldShatterActor::StartShatter(UStaticMesh* ShardMesh, UMaterialInterface* ShardMaterial, float Radius,
	int32 ShardCount, float Lifetime, float Speed, const FVector& PushDirection)
{
	TotalLifetime = FMath::Max(0.1f, Lifetime);
	Elapsed = 0.0f;
	SetLifeSpan(TotalLifetime + 0.1f);

	if (!ShardMesh || ShardCount <= 0)
	{
		return;
	}

	const float GoldenAngle = PI * (3.0f - FMath::Sqrt(5.0f));
	const float SizeScale = LeeShieldShatter::ShardSizeAt110 * Radius / 110.0f;

	for (int32 Index = 0; Index < ShardCount; ++Index)
	{
		// 피보나치 구: 위(+Z)에서 아래로 내려가며 점을 고르게 찍는다
		const float Z = 1.0f - 2.0f * (Index + 0.5f) / ShardCount;
		const float Ring = FMath::Sqrt(FMath::Max(0.0f, 1.0f - Z * Z));
		const float Phi = Index * GoldenAngle;
		const FVector Dir = FVector(FMath::Cos(Phi) * Ring, FMath::Sin(Phi) * Ring, Z).GetSafeNormal();

		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this);
		Mesh->SetStaticMesh(ShardMesh);
		Mesh->SetMaterial(0, ShardMaterial);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetupAttachment(GetRootComponent());
		Mesh->RegisterComponent();

		// 조각 메시는 +X가 바깥을 본다 → 표면 방향으로 세우고 임의로 돌려 모양을 다양하게
		FRotator Rotation = FRotationMatrix::MakeFromX(Dir).Rotator();
		Rotation.Roll = FMath::FRandRange(0.0f, 360.0f);
		const FVector Scale(1.0f, SizeScale * FMath::FRandRange(0.8f, 1.25f), SizeScale * FMath::FRandRange(0.8f, 1.25f));
		Mesh->SetRelativeLocationAndRotation(Dir * Radius, Rotation);
		Mesh->SetRelativeScale3D(Scale);

		FLeeShieldShard Shard;
		Shard.Mesh = Mesh;
		Shard.StartScale = Scale;
		Shard.Velocity = Dir * FMath::FRandRange(Speed * 0.6f, Speed)
			+ PushDirection * Speed * 0.35f
			+ FVector(0.0f, 0.0f, Speed * 0.25f);
		Shard.SpinPerSecond = FRotator(FMath::FRandRange(-360.0f, 360.0f), FMath::FRandRange(-360.0f, 360.0f), FMath::FRandRange(-360.0f, 360.0f));
		Shards.Add(Shard);
	}
}

// 매 프레임 — 조각 이동(중력 포함)·회전, 수명 후반에는 작아지며 사라진다
void ALeeShieldShatterActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;
	const float LifeRatio = Elapsed / TotalLifetime;
	const float ShrinkAlpha = FMath::Clamp((LifeRatio - LeeShieldShatter::FadeStartRatio) / (1.0f - LeeShieldShatter::FadeStartRatio), 0.0f, 1.0f);

	for (FLeeShieldShard& Shard : Shards)
	{
		if (!Shard.Mesh)
		{
			continue;
		}
		Shard.Velocity.Z -= LeeShieldShatter::Gravity * DeltaSeconds;
		Shard.Mesh->AddRelativeLocation(Shard.Velocity * DeltaSeconds);
		Shard.Mesh->AddLocalRotation(Shard.SpinPerSecond * DeltaSeconds);
		Shard.Mesh->SetRelativeScale3D(Shard.StartScale * (1.0f - ShrinkAlpha));
	}
}
