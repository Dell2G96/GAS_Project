// 26.11.10 신규 - Enemy AOE 공격의 바닥 예고 원 + 폭발 판정 액터

#include "LeeAoeTelegraphActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/OverlapResult.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
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
}

// 매 프레임 예고 원을 채우고, 서버는 시간이 다 되면 폭발시킨다
void ALeeAoeTelegraphActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bExploded)
	{
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

// [서버] 원 안의 Pawn마다 한 번씩: 아군 제외 → 데미지 Spec 적용 → (퍼펙트 가드가 아니면) 중심에서 바깥으로 넉백
void ALeeAoeTelegraphActor::Explode()
{
	bExploded = true;

	AActor* InstigatorActor = InstigatorActorRef.Get();
	if (InstigatorActor && DamageSpec.IsValid())
	{
		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(LeeAoeExplode), false, this);
		Params.AddIgnoredActor(InstigatorActor);
		GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity,
			FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius), Params);

		ULeeTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<ULeeTeamSubsystem>();
		TSet<AActor*> HitActors;

		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Victim = Overlap.GetActor();
			if (!Victim || HitActors.Contains(Victim))
			{
				continue;
			}
			HitActors.Add(Victim);

			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim);
			if (!TargetASC || (TeamSubsystem && !TeamSubsystem->CanCauseDamage(InstigatorActor, Victim)))
			{
				continue;
			}

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
		}

		UE_LOG(LogTemp, Log, TEXT("[Aoe] Explode instigator=%s radius=%.0f hit=%d"), *GetNameSafe(InstigatorActor), Radius, HitActors.Num());
	}
	else
	{
		// 공격자가 사라졌으면 아군 판정을 할 수 없으므로 판정 없이 끝낸다
		UE_LOG(LogTemp, Log, TEXT("[Aoe] 공격자 없음 또는 Spec 없음 — 판정 없이 제거"));
	}

	Multicast_PlayExplosion();
	SetLifeSpan(LingerTimeAfterExplosion);
}

// [모두] 예고 원을 끄고 폭발 이펙트를 낸다
void ALeeAoeTelegraphActor::Multicast_PlayExplosion_Implementation()
{
	bExploded = true;
	TelegraphDecal->SetVisibility(false);

	if (ExplosionEffect)
	{
		const float Scale = Radius / ExplosionEffectBaseRadius;
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionEffect, GetActorLocation(), FRotator::ZeroRotator, FVector(Scale));
	}

	BP_OnExploded();
}
