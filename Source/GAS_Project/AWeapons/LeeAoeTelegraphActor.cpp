// 26.11.10 신규 - Enemy AOE 공격의 바닥 예고 원 + 폭발 판정 액터

#include "LeeAoeTelegraphActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/OverlapResult.h"
#include "GAS_Project/AAbilitySystem/Abilities/LeeGameplayAbility_ZoneForce.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ATeam/LeeTeamSubsystem.h"

// 리플리케이트 설정 + 아래를 향해 투영하는 데칼 생성
ALeeAoeTelegraphActor::ALeeAoeTelegraphActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TelegraphDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("TelegraphDecal"));
	TelegraphDecal->SetupAttachment(SceneRoot);
	TelegraphDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); // 데칼의 X축이 아래를 보게 해 바닥에 투영

	// 26.11.12 추가 - 폭발 판정용 충돌 구: Pawn과만 Overlap, 폭발 전에는 꺼 둔다
	HitSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HitSphere"));
	HitSphere->SetupAttachment(SceneRoot);
	HitSphere->InitSphereRadius(1.0f);
	HitSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HitSphere->SetCollisionObjectType(ECC_WorldDynamic);
	HitSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HitSphere->SetGenerateOverlapEvents(true);
	HitSphere->SetCanEverAffectNavigation(false);
	HitSphere->SetHiddenInGame(true);
}

// 서버가 스폰 직후 값을 넣는다 (Radius·TelegraphTime은 첫 복제 때 클라로 간다)
void ALeeAoeTelegraphActor::InitAoe(const FGameplayEffectSpecHandle& InDamageSpec, AActor* InInstigatorActor, float InRadius,
	float InTelegraphTime, const FLeeKnockbackParams& InKnockback)
{
	DamageSpec = InDamageSpec;
	InstigatorActorRef = InInstigatorActor;
	Radius = FMath::Max(InRadius, 10.0f);
	TelegraphTime = FMath::Max(InTelegraphTime, 0.05f);
	Knockback = InKnockback;
}

// 범위·예고 시간은 스폰 때 한 번만 보내면 된다
void ALeeAoeTelegraphActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ALeeAoeTelegraphActor, Radius, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ALeeAoeTelegraphActor, TelegraphTime, COND_InitialOnly);
}

// 데칼 크기를 범위에 맞추고 채움 정도를 바꿀 다이내믹 머티리얼을 만든다
void ALeeAoeTelegraphActor::BeginPlay()
{
	Super::BeginPlay();

	TelegraphDecal->DecalSize = FVector(DecalHalfDepth, Radius, Radius);
	DecalMaterial = TelegraphDecal->CreateDynamicMaterialInstance();
	if (DecalMaterial)
	{
		DecalMaterial->SetScalarParameterValue(ProgressParameterName, 0.0f);
	}

	// 26.11.12 추가 - 판정은 서버만 한다
	if (HasAuthority())
	{
		HitSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnHitSphereBeginOverlap);
	}
}

// 26.11.12 추가 - 액터가 사라질 때(수명 끝) 폭발 이펙트의 새 파티클 생성을 멈춘다. 남은 파티클은 자연스럽게 사라진 뒤 자동 삭제
void ALeeAoeTelegraphActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ExplosionComponent)
	{
		ExplosionComponent->Deactivate();
		ExplosionComponent = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

// 매 프레임 예고 원을 채우고, 서버는 시간이 다 되면 폭발시킨다. 폭발 뒤에는 충돌 구를 키운다
void ALeeAoeTelegraphActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bExploded)
	{
		if (bHitActive)
		{
			UpdateHitSphere(DeltaSeconds);
		}
		if (bZoneActive)
		{
			UpdateZone(DeltaSeconds);
		}
		return;
	}

	ElapsedTime += DeltaSeconds;
	const float Progress = FMath::Clamp(ElapsedTime / TelegraphTime, 0.0f, 1.0f);
	if (DecalMaterial)
	{
		DecalMaterial->SetScalarParameterValue(ProgressParameterName, Progress);
	}

	if (HasAuthority() && ElapsedTime >= TelegraphTime)
	{
		Explode();
	}
}

// [서버] 폭발 시작: 모든 화면에 이펙트를 내고, 충돌 구 판정(중심에서 바깥으로 커짐)을 시작한다
void ALeeAoeTelegraphActor::Explode()
{
	bExploded = true;
	Multicast_PlayExplosion();

	if (InstigatorActorRef.IsValid() && DamageSpec.IsValid())
	{
		bHitActive = true;
		HitElapsedTime = 0.0f;
		HitActors.Reset();

		// 26.11.12 추가 - 장판: 폭발 후 ZoneDuration 동안 틱 데미지·밀기/당기기
		bZoneActive = ZoneDuration > 0.0f;
		ZoneElapsedTime = 0.0f;
		ZoneTickRemaining = 0.0f; // 첫 프레임에 바로 한 번 틱(힘 걸기). 틱 데미지는 두 번째 틱부터 (폭발 데미지와 겹치지 않게)
		ForcedActors.Reset();

		UE_LOG(LogTemp, Log, TEXT("[Aoe] Explode instigator=%s radius=%.0f 판정 시작(지연 %.2f초, 확장 %.2f초) 장판 %.1f초 힘 %.0f"),
			*GetNameSafe(InstigatorActorRef.Get()), Radius, HitStartDelay, HitExpandTime, ZoneDuration, ZoneForceStrength);
	}
	else
	{
		// 공격자가 사라졌으면 아군 판정을 할 수 없으므로 판정 없이 끝낸다
		UE_LOG(LogTemp, Log, TEXT("[Aoe] 공격자 없음 또는 Spec 없음 — 판정 없이 제거"));
	}

	// 이펙트가 끝까지 재생되고, 충돌 구 판정·장판이 끝날 때까지 남겨 둔다
	SetLifeSpan(FMath::Max3(LingerTimeAfterExplosion, HitStartDelay + HitExpandTime + 0.2f, ZoneDuration + 0.1f));
}

// 26.11.12 추가 - [서버] 장판 시간을 세고, 틱 간격이 되면 ZoneTick. 시간이 다 되면 장판 끝
void ALeeAoeTelegraphActor::UpdateZone(float DeltaSeconds)
{
	ZoneElapsedTime += DeltaSeconds;
	if (ZoneElapsedTime >= ZoneDuration)
	{
		bZoneActive = false;
		return;
	}

	ZoneTickRemaining -= DeltaSeconds;
	if (ZoneTickRemaining <= 0.0f)
	{
		ZoneTickRemaining += ZoneTickInterval;
		ZoneTick();
	}
}

// 26.11.12 추가 - [서버] 장판 안의 적 찾기 → 처음 들어온 적에게 남은 시간만큼 힘 걸기, 두 번째 틱부터 틱 데미지
void ALeeAoeTelegraphActor::ZoneTick()
{
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LeeAoeZone), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius), Params);

	const bool bFirstTick = ZoneElapsedTime <= ZoneTickInterval * 0.5f;
	TSet<AActor*> Visited;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Victim = Overlap.GetActor();
		if (!Victim || Visited.Contains(Victim) || !CanAffect(Victim))
		{
			continue;
		}
		Visited.Add(Victim);

		// 힘은 장판 반경 안에서만 작용하므로 대상마다 한 번만 걸면 된다 (남은 장판 시간 동안 유지)
		if (!FMath::IsNearlyZero(ZoneForceStrength) && !ForcedActors.Contains(Victim))
		{
			ForcedActors.Add(Victim);
			ULeeGameplayAbility_ZoneForce::SendZoneForceEvent(InstigatorActorRef.Get(), Victim, GetActorLocation(), Radius,
				ZoneForceStrength, ZoneDuration - ZoneElapsedTime);
		}

		// 틱 데미지: 폭발 데미지 Spec을 복사해 크기만 바꾼다
		if (!bFirstTick && ZoneTickDamage > 0.0f)
		{
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim))
			{
				FGameplayEffectSpec TickSpec(*DamageSpec.Data.Get());
				TickSpec.SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Damage, -ZoneTickDamage);
				TargetASC->ApplyGameplayEffectSpecToSelf(TickSpec);
			}
		}
	}
}

// 26.11.12 추가 - [서버] 공격자 자신·아군이 아니고 ASC가 있는 대상인지
bool ALeeAoeTelegraphActor::CanAffect(AActor* Victim) const
{
	AActor* InstigatorActor = InstigatorActorRef.Get();
	if (!Victim || !InstigatorActor || Victim == InstigatorActor || !DamageSpec.IsValid()
		|| !UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim))
	{
		return false;
	}

	ULeeTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<ULeeTeamSubsystem>();
	return !TeamSubsystem || TeamSubsystem->CanCauseDamage(InstigatorActor, Victim);
}

// [서버] 지연 시간이 지나면 충돌 구를 켜고, HitExpandTime 동안 반경을 0 → Radius로 키운다 (커질 때마다 새로 닿은 Pawn에 Overlap 이벤트)
void ALeeAoeTelegraphActor::UpdateHitSphere(float DeltaSeconds)
{
	HitElapsedTime += DeltaSeconds;
	if (HitElapsedTime < HitStartDelay)
	{
		return;
	}

	const float Alpha = (HitExpandTime > KINDA_SMALL_NUMBER)
		? FMath::Clamp((HitElapsedTime - HitStartDelay) / HitExpandTime, 0.0f, 1.0f)
		: 1.0f;

	if (HitSphere->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
	{
		HitSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	HitSphere->SetSphereRadius(FMath::Max(Radius * Alpha, 1.0f), /*bUpdateOverlaps*/ true);

	if (Alpha >= 1.0f)
	{
		bHitActive = false;
		HitSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UE_LOG(LogTemp, Log, TEXT("[Aoe] 판정 끝 instigator=%s hit=%d"), *GetNameSafe(InstigatorActorRef.Get()), HitActors.Num());
	}
}

// [서버] 충돌 구가 Pawn 캡슐에 처음 닿는 순간 = 피격 순간
void ALeeAoeTelegraphActor::OnHitSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority() && bHitActive)
	{
		ApplyHit(OtherActor);
	}
}

// [서버] 대상마다 한 번만: 공격자·아군 제외 → 데미지 Spec 적용 → (퍼펙트 가드가 아니면) 중심에서 바깥으로 넉백
void ALeeAoeTelegraphActor::ApplyHit(AActor* Victim)
{
	AActor* InstigatorActor = InstigatorActorRef.Get();
	if (!Victim || !InstigatorActor || Victim == InstigatorActor || HitActors.Contains(Victim) || !DamageSpec.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim);
	ULeeTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<ULeeTeamSubsystem>();
	if (!TargetASC || (TeamSubsystem && !TeamSubsystem->CanCauseDamage(InstigatorActor, Victim)))
	{
		return;
	}
	HitActors.Add(Victim);

	// 퍼펙트 가드 여부는 데미지 적용 전에 기록 (투사체와 같은 규칙)
	const bool bUnblockable = DamageSpec.Data->GetDynamicAssetTags().HasTagExact(MyTags::Souls::DamageType_Attack_Unblockable);
	const bool bPerfectGuard = !bUnblockable && TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Guard_Perfect);

	TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());

	if (!bPerfectGuard)
	{
		const FVector Outward = (Victim->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		const float OutwardYaw = Outward.IsNearlyZero() ? InstigatorActor->GetActorRotation().Yaw : Outward.Rotation().Yaw;
		ULeeGameplayAbility_Knockback::SendKnockbackEvent(InstigatorActor, Victim, Knockback, OutwardYaw);
	}

	UE_LOG(LogTemp, Log, TEXT("[Aoe] Hit victim=%s t=%.2f초(폭발 후)"), *GetNameSafe(Victim), HitElapsedTime);
}

// [모두] 예고 원을 끄고 폭발 이펙트를 낸다
void ALeeAoeTelegraphActor::Multicast_PlayExplosion_Implementation()
{
	bExploded = true;
	TelegraphDecal->SetVisibility(false);

	if (ExplosionEffect)
	{
		// 26.11.12 수정 - 바닥에서 살짝 띄우고, EndPlay에서 멈출 수 있게 컴포넌트를 보관한다
		const float Scale = Radius / ExplosionEffectBaseRadius;
		ExplosionComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionEffect,
			GetActorLocation() + ExplosionEffectOffset, FRotator::ZeroRotator, FVector(Scale));
	}

	BP_OnExploded();
}
