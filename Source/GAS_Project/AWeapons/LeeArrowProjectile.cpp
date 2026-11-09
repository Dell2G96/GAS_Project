// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeArrowProjectile.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

// 손에 든 상태로 시작 — 이동·충돌·수명 모두 꺼 두고 Launch에서 켠다
ALeeArrowProjectile::ALeeArrowProjectile()
{
	// 부모의 테스트용 구체는 쓰지 않는다
	VisualMesh->SetHiddenInGame(true);
	VisualMesh->SetVisibility(false);

	CollisionComponent->InitSphereRadius(6.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	ArrowMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ArrowMesh"));
	ArrowMesh->SetupAttachment(CollisionComponent);
	ArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArrowMesh->SetCastShadow(false);

	TrailFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailFX"));
	TrailFX->SetupAttachment(ArrowMesh);
	TrailFX->SetAutoActivate(false);

	// 손에 들고 있는 동안은 사라지면 안 된다 (Launch에서 사거리 기준으로 다시 정함)
	InitialLifeSpan = 0.0f;
}

void ALeeArrowProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, bLaunched);
}

// [서버] 손에서 떼어 날린다. 속도·사거리는 활 어빌리티가 차지 비율로 계산해 넘겨준다
void ALeeArrowProjectile::Launch(const FVector& Direction, float Speed, float Range)
{
	if (!HasAuthority() || bLaunched)
	{
		return;
	}

	const FVector Dir = Direction.GetSafeNormal();
	const float SafeSpeed = FMath::Max(Speed, 100.0f);

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorRotation(Dir.Rotation());

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	ProjectileMovement->ProjectileGravityScale = ArrowGravityScale;
	ProjectileMovement->InitialSpeed = SafeSpeed;
	ProjectileMovement->MaxSpeed = SafeSpeed;
	ProjectileMovement->Velocity = Dir * SafeSpeed;
	ProjectileMovement->SetUpdatedComponent(CollisionComponent);
	ProjectileMovement->Activate(true);

	// 사거리 = 속도 × 시간 → 사거리만큼 날면 사라진다
	SetLifeSpan(FMath::Max(Range, 100.0f) / SafeSpeed);

	bLaunched = true;
	OnRep_Launched();
}

// 발사 상태가 되면 꼬리 이펙트를 켠다 (서버는 직접, 클라는 리플리케이션으로 호출)
void ALeeArrowProjectile::OnRep_Launched()
{
	if (bLaunched && TrailFX && TrailFX->GetAsset())
	{
		TrailFX->Activate(true);
	}
}

// 날아간 화살이 사라질 때만 그 자리에 명중 이펙트 (조준 취소로 사라지는 손 화살은 제외)
void ALeeArrowProjectile::Destroyed()
{
	if (bLaunched && ImpactFX && GetNetMode() != NM_DedicatedServer)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactFX, GetActorLocation(), GetActorRotation());
	}

	Super::Destroyed();
}
