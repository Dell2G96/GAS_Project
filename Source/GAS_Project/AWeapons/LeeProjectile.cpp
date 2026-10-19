// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GAS_Project/ATeam/LeeTeamSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ALeeProjectile::ALeeProjectile()
{
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(12.0f);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionComponent->OnComponentHit.AddDynamic(this, &ALeeProjectile::OnProjectileHit);
	SetRootComponent(CollisionComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 1800.0f;
	ProjectileMovement->MaxSpeed = 1800.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	// [테스트용] 2026.10.20 — 눈으로 보이는 기본 구체. 충돌/판정에는 관여하지 않는다(NoCollision).
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(CollisionComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		VisualMesh->SetStaticMesh(SphereMeshFinder.Object);
		VisualMesh->SetWorldScale3D(FVector(0.24f)); // 기본 구체(반지름 50) -> 대략 반지름 12로 축소
	}

	InitialLifeSpan = 3.0f;
}

void ALeeProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (InstigatorActorRef)
	{
		CollisionComponent->IgnoreActorWhenMoving(InstigatorActorRef, true);
	}
}

void ALeeProjectile::InitProjectile(const FGameplayEffectSpecHandle& InDamageSpec, AActor* InInstigatorActor)
{
	DamageSpec = InDamageSpec;
	InstigatorActorRef = InInstigatorActor;
}

// 서버 권위에서만 피해 판정 — 클라이언트는 리플리케이트된 이동만 본다
void ALeeProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!OtherActor || OtherActor == this || OtherActor == InstigatorActorRef)
	{
		return;
	}

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
	{
		// 아군 오사 방지 — 플레이어용 활/화살과 동일한 규칙 재사용
		if (ULeeTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<ULeeTeamSubsystem>())
		{
			if (!TeamSubsystem->CanCauseDamage(InstigatorActorRef, OtherActor))
			{
				Destroy();
				return;
			}
		}

		if (DamageSpec.IsValid())
		{
			DamageSpec.Data->GetContext().AddHitResult(Hit);
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());
		}
	}

	// 피해 대상이든 벽이든, 맞으면 소멸한다
	Destroy();
}
