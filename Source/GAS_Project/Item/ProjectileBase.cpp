// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"


AProjectileBase::AProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ProjectileCollisionBox"));
	SetRootComponent(ProjectileCollisionBox);
	ProjectileCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	ProjectileCollisionBox->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnProjectileHit);
	ProjectileCollisionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnProjectileBeginOverlap);

	ProjectileNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileNiagaraComponent"));
	ProjectileNiagaraComponent->SetupAttachment(GetRootComponent());

	ProjectileMovementComponent= CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->InitialSpeed = 700.f;
	ProjectileMovementComponent->MaxSpeed = 900.f;
	ProjectileMovementComponent->Velocity = FVector(1.f, 0.f, 0.f);
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;

	InitialLifeSpan = 4.f;
	
}

void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (ProjectileDamagePolicy ==EProjectileDamagePolicy::OnBeginOverlap)
	{
		ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	}
	
}

void AProjectileBase::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	BP_OnSpawnProjectileHitFX(Hit.ImpactPoint);

	APawn* HitPawn = Cast<APawn>(OtherActor);

	if (!HitPawn || !UCAbilitySystemStatics::IsTargetPawnHostile(GetInstigator(), HitPawn))
	{
		Destroy();
		return;
	}

	bool bIsValidBlock = false;

	const bool bIsPlayerBlocking = UCAbilitySystemStatics::NativeDoseActorHaveTag(HitPawn, MyTags::Status::Guarding);

	if (bIsPlayerBlocking)
	{
		bIsValidBlock = UCAbilitySystemStatics::IsValidBlock(this, HitPawn);
	}

	FGameplayEventData EventData;
	//EventData.EventTag = MyTags::Events::Hit::LightHit;
	EventData.Instigator = this;
	EventData.Target = HitPawn;

	if (bIsValidBlock)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitPawn, MyTags::Events::Block_Hit, EventData);
	}
	else
	{
		UCAbilitySystemStatics::SendDamageEventToPlayer(HitPawn, DamageEffect, EventData, MyTags::SetByCaller::Projectile, Damage,MyTags::None);
	}
	Destroy();
	
}

void AProjectileBase::OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OverlappedActors.Contains(OtherActor))
	{
		return;
	}

	OverlappedActors.AddUnique(OtherActor);

	if (APawn* HitPawn = Cast<APawn>(OtherActor))
	{
		FGameplayEventData Data;
		Data.Instigator = this;
		Data.Target = HitPawn;

		if (UCAbilitySystemStatics::IsTargetPawnHostile(GetInstigator(), HitPawn))
		{
			UCAbilitySystemStatics::SendDamageEventToPlayer(HitPawn, DamageEffect, Data, MyTags::SetByCaller::Projectile, Damage,MyTags::None);
		}
	}
}





