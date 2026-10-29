// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GAS_Project/MyTags.h"
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

// 명중 시 사용할 넉백 값 저장 (발사 직전 GA가 호출)
void ALeeProjectile::InitKnockback(const FVector& InOffset, float InDuration, float InGuardScale)
{
	KnockbackOffset = InOffset;
	KnockbackDuration = InDuration;
	GuardKnockbackScale = InGuardScale;
}

// [넉백][서버] 투사체 비행 방향(수평) 기준 오프셋을 월드 오프셋으로 변환해 피해자에게 이벤트 전송
void ALeeProjectile::SendKnockbackEvent(AActor* HitActor, const UAbilitySystemComponent* HitASC, bool bPerfectGuard) const
{
	if (bPerfectGuard || KnockbackOffset.IsNearlyZero() || KnockbackDuration <= 0.0f)
	{
		return;
	}

	// 사망 중인 대상은 밀지 않는다 (사망 몽타주 우선)
	if (HitASC->HasMatchingGameplayTag(MyTags::Souls::Status_Death_Dying))
	{
		return;
	}

	// 가드 중이면 가드 배율 적용
	const float Scale = HitASC->HasMatchingGameplayTag(MyTags::Souls::Status_Guard_Active) ? GuardKnockbackScale : 1.0f;
	if (Scale <= 0.0f)
	{
		return;
	}

	// bRotationFollowsVelocity=true라 액터 회전 = 비행 방향. Yaw만 사용해 X/Y는 지면과 평행
	const FRotator FlightYaw(0.0f, GetActorRotation().Yaw, 0.0f);
	const FVector WorldOffset = FlightYaw.RotateVector(KnockbackOffset * Scale);

	// 근접과 동일 규약 — 월드 오프셋은 TargetData(LocationInfo)에, 이동 시간은 EventMagnitude에 싣는다
	FGameplayAbilityTargetData_LocationInfo* OffsetData = new FGameplayAbilityTargetData_LocationInfo();
	OffsetData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	OffsetData->TargetLocation.LiteralTransform = FTransform(WorldOffset);

	FGameplayEventData Payload;
	Payload.EventTag = MyTags::Souls::Event_Combat_Knockback;
	Payload.Instigator = InstigatorActorRef;
	Payload.Target = HitActor;
	Payload.EventMagnitude = KnockbackDuration;
	Payload.TargetData.Add(OffsetData);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, MyTags::Souls::Event_Combat_Knockback, Payload);
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
			// [넉백] 퍼펙트 가드 여부는 데미지 적용 전에 기록 (가드불가 공격은 퍼펙트 가드가 통하지 않음)
			const bool bUnblockable = DamageSpec.Data->GetDynamicAssetTags().HasTagExact(MyTags::Souls::DamageType_Attack_Unblockable);
			const bool bPerfectGuard = !bUnblockable && TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Guard_Perfect);

			DamageSpec.Data->GetContext().AddHitResult(Hit);
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());

			SendKnockbackEvent(OtherActor, TargetASC, bPerfectGuard);
		}
	}

	// 피해 대상이든 벽이든, 맞으면 소멸한다
	Destroy();
}
