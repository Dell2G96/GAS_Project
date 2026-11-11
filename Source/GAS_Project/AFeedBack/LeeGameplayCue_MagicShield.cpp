// 26.11.08 신규 - 마법 쉴드 표시 GameplayCue (GameplayCue.Souls.Shield.Active)
// 26.11.11 수정 - 에셋 팩 BP_Shield_Main 참고: 추가 이펙트 구·바닥 데칼·등장/소멸 페이드·피격 파동 추가

#include "LeeGameplayCue_MagicShield.h"

#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AFeedBack/LeeShieldShatterActor.h"

namespace
{
	/** 엔진 구(/Engine/BasicShapes/Sphere) 반지름(cm) — 쉴드 메시 크기 기준 */
	constexpr float ShieldMeshBaseRadius = 50.0f;

	/** 파동 구를 본체보다 살짝 크게 그려 겹침 깜빡임을 막는다 */
	constexpr float HitRippleMeshScale = 1.01f;
}

// 생성자 — Cue 태그 지정, 제거 시 자동 파괴, 기본 메시는 엔진 구
ALeeGameplayCue_MagicShield::ALeeGameplayCue_MagicShield()
{
	GameplayCueTag = MyTags::Souls::GameplayCue_Shield_Active;
	bAutoDestroyOnRemove = true;

	// 페이드·파동이 있을 때만 켠다
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		ShieldMesh = SphereMesh.Object;
	}

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
}

// 쉴드 메시 컴포넌트를 만들어 쉴드 크기(CurrentRadius × 배율)로 붙인다
UStaticMeshComponent* ALeeGameplayCue_MagicShield::CreateShieldMeshComponent(FName Name, float ScaleMultiplier)
{
	UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), Name));
	MeshComponent->SetStaticMesh(ShieldMesh);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCastShadow(false);
	MeshComponent->SetupAttachment(GetRootComponent());
	MeshComponent->RegisterComponent();
	MeshComponent->SetRelativeScale3D(FVector(CurrentRadius / ShieldMeshBaseRadius * ScaleMultiplier));
	return MeshComponent;
}

// 쉴드 켜짐 — 타깃에 붙어 본체·추가 구·데칼을 만들고 금 0, 투명 상태에서 페이드 인 시작
bool ALeeGameplayCue_MagicShield::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (!MyTarget || !ShieldMesh)
	{
		return false;
	}

	// 사라지는 중에 다시 켜진 경우 남은 컴포넌트부터 정리
	DestroyShieldComponents();

	AttachToActor(MyTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	// 캐릭터 캡슐을 덮을 만큼 키운다
	float Radius = ShieldRadius;
	if (const ACharacter* Character = Cast<ACharacter>(MyTarget))
	{
		Radius = FMath::Max(Radius, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 1.1f);
	}
	CurrentRadius = Radius;

	// 본체 (균열·번쩍임)
	ShieldMeshComponent = CreateShieldMeshComponent(TEXT("ShieldMesh"), 1.0f);
	if (ShieldMaterial)
	{
		ShieldMID = UMaterialInstanceDynamic::Create(ShieldMaterial, this);
		ShieldMeshComponent->SetMaterial(0, ShieldMID);
	}

	// 추가 이펙트 구 (팩 AdditionalEffect)
	if (OverlayMaterial)
	{
		OverlayMeshComponent = CreateShieldMeshComponent(TEXT("OverlayMesh"), 1.0f);
		OverlayMID = UMaterialInstanceDynamic::Create(OverlayMaterial, this);
		OverlayMID->SetVectorParameterValue(TEXT("Color_1"), OverlayColor);
		OverlayMeshComponent->SetMaterial(0, OverlayMID);
	}

	// 바닥 데칼 (팩 Decal) — 쉴드 중심에서 아래로 투영, 깊이는 반지름보다 넉넉히 잡아 바닥에 닿게 한다
	if (DecalMaterial)
	{
		DecalComponent = NewObject<UDecalComponent>(this, MakeUniqueObjectName(this, UDecalComponent::StaticClass(), TEXT("ShieldDecal")));
		DecalComponent->SetupAttachment(GetRootComponent());
		DecalComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
		DecalComponent->DecalSize = FVector(Radius * 1.2f, Radius * DecalSizeScale, Radius * DecalSizeScale);
		DecalComponent->RegisterComponent();
		DecalMID = UMaterialInstanceDynamic::Create(DecalMaterial, this);
		DecalMID->SetVectorParameterValue(TEXT("Color1"), DecalColor);
		DecalComponent->SetDecalMaterial(DecalMID);
	}

	// 금 없이 시작
	NumCrackImpacts = 0;
	NextCrackSlot = 0;
	CrackSpread = 0.0f;
	UpdateCrackParameters();

	// 페이드 인 (시간이 0이면 바로 다 보인다)
	FadeTarget = 1.0f;
	if (FadeInTime > 0.0f)
	{
		FadeAlpha = 0.0f;
		FadeSpeed = 1.0f / FadeInTime;
		SetActorTickEnabled(true);
	}
	else
	{
		FadeAlpha = 1.0f;
	}
	ApplyFadeAlpha(FadeAlpha);
	return true;
}

// 쉴드 꺼짐 — 페이드 아웃 시작, 액터 파괴는 FadeOutTime만큼 늦춘다 (부모가 OnRemove 뒤에 AutoDestroyDelay를 읽는다)
bool ALeeGameplayCue_MagicShield::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	GetWorldTimerManager().ClearTimer(HitFlashTimer);

	if (FadeOutTime > 0.0f && ShieldMeshComponent)
	{
		FadeTarget = 0.0f;
		FadeSpeed = 1.0f / FadeOutTime;
		AutoDestroyDelay = FadeOutTime;
		SetActorTickEnabled(true);
	}
	else
	{
		AutoDestroyDelay = 0.0f;
		DestroyShieldComponents();
	}
	return true;
}

// 페이드와 피격 파동을 진행하고, 둘 다 끝나면 Tick을 끈다
void ALeeGameplayCue_MagicShield::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	bool bFading = false;
	if (!FMath::IsNearlyEqual(FadeAlpha, FadeTarget))
	{
		FadeAlpha = FMath::FInterpConstantTo(FadeAlpha, FadeTarget, DeltaSeconds, FadeSpeed);
		ApplyFadeAlpha(FadeAlpha);
		bFading = !FMath::IsNearlyEqual(FadeAlpha, FadeTarget);

		// 다 사라졌으면 컴포넌트 정리 (액터는 AutoDestroyDelay 뒤에 정리된다)
		if (!bFading && FadeTarget <= 0.0f)
		{
			DestroyShieldComponents();
		}
	}

	for (int32 Index = HitRipples.Num() - 1; Index >= 0; --Index)
	{
		FLeeShieldHitRipple& Ripple = HitRipples[Index];
		Ripple.Elapsed += DeltaSeconds;
		if (Ripple.Elapsed >= HitRippleDuration || !Ripple.Mesh)
		{
			if (Ripple.Mesh)
			{
				Ripple.Mesh->DestroyComponent();
			}
			HitRipples.RemoveAt(Index);
			continue;
		}
		UpdateHitRipple(Ripple);
	}

	if (!bFading && HitRipples.Num() == 0)
	{
		SetActorTickEnabled(false);
	}
}

// 본체(Appear)·추가 구·데칼(MasterOpacity)의 보이는 정도를 맞춘다
void ALeeGameplayCue_MagicShield::ApplyFadeAlpha(float Alpha)
{
	if (ShieldMID)
	{
		ShieldMID->SetScalarParameterValue(AppearParameterName, Alpha);
	}
	if (OverlayMID)
	{
		OverlayMID->SetScalarParameterValue(MasterOpacityParameterName, Alpha);
	}
	if (DecalMID)
	{
		DecalMID->SetScalarParameterValue(MasterOpacityParameterName, Alpha);
	}
}

// 쉴드가 만든 컴포넌트(본체·추가 구·데칼·파동)를 모두 지운다
void ALeeGameplayCue_MagicShield::DestroyShieldComponents()
{
	for (FLeeShieldHitRipple& Ripple : HitRipples)
	{
		if (Ripple.Mesh)
		{
			Ripple.Mesh->DestroyComponent();
		}
	}
	HitRipples.Reset();

	if (ShieldMeshComponent)
	{
		ShieldMeshComponent->DestroyComponent();
		ShieldMeshComponent = nullptr;
	}
	if (OverlayMeshComponent)
	{
		OverlayMeshComponent->DestroyComponent();
		OverlayMeshComponent = nullptr;
	}
	if (DecalComponent)
	{
		DecalComponent->DestroyComponent();
		DecalComponent = nullptr;
	}
	ShieldMID = nullptr;
	OverlayMID = nullptr;
	DecalMID = nullptr;
}

// 맞은 방향에 금 자리를 추가(가까운 기존 자리는 재사용)하고, 금이 퍼진 정도를 CrackAmount에 맞춘다
void ALeeGameplayCue_MagicShield::AddCrackImpact(const FVector& WorldDirection, float CrackAmount)
{
	if (!ShieldMeshComponent)
	{
		return;
	}

	// 머티리얼은 메시 기준 위치(LocalPosition)로 그리므로 방향도 메시 기준으로 바꾼다
	const FVector LocalDirection = ShieldMeshComponent->GetComponentTransform()
		.InverseTransformVectorNoScale(WorldDirection).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);

	const float MergeCos = FMath::Cos(FMath::DegreesToRadians(CrackMergeAngle));
	bool bMerged = false;
	for (int32 Index = 0; Index < NumCrackImpacts; ++Index)
	{
		if (FVector::DotProduct(CrackDirections[Index], LocalDirection) >= MergeCos)
		{
			bMerged = true;
			break;
		}
	}

	if (!bMerged)
	{
		// 4곳이 다 찼으면 가장 오래된 자리부터 덮어쓴다
		CrackDirections[NextCrackSlot] = LocalDirection;
		NextCrackSlot = (NextCrackSlot + 1) % MaxCrackImpacts;
		NumCrackImpacts = FMath::Min(NumCrackImpacts + 1, MaxCrackImpacts);
	}

	CrackSpread = FMath::Lerp(MinCrackSpread, 1.0f, FMath::Clamp(CrackAmount, 0.0f, 1.0f));
	UpdateCrackParameters();
}

// 금 자리 4곳을 머티리얼에 넣는다 — 쓰지 않는 자리는 w = 0(안 보임)
void ALeeGameplayCue_MagicShield::UpdateCrackParameters()
{
	if (!ShieldMID)
	{
		return;
	}
	for (int32 Index = 0; Index < MaxCrackImpacts; ++Index)
	{
		const bool bUsed = Index < NumCrackImpacts;
		const FVector Direction = bUsed ? CrackDirections[Index] : FVector::ForwardVector;
		const FLinearColor Value(Direction.X, Direction.Y, Direction.Z, bUsed ? CrackSpread : 0.0f);
		ShieldMID->SetVectorParameterValue(*FString::Printf(TEXT("Hit%d"), Index), Value);
	}
}

// 맞은 방향(월드)의 쉴드 표면 위치
FVector ALeeGameplayCue_MagicShield::GetSurfacePoint(const FVector& WorldDirection) const
{
	const FVector Center = ShieldMeshComponent ? ShieldMeshComponent->GetComponentLocation() : GetActorLocation();
	return Center + WorldDirection.GetSafeNormal() * CurrentRadius;
}

// 피격 파동 시작 — 쉴드 구를 하나 더 겹쳐 MI_Hit 머티리얼로 맞은 자리에서 원이 퍼지게 한다
void ALeeGameplayCue_MagicShield::PlayHitRipple(const FVector& WorldDirection)
{
	if (!HitRippleMaterial || !ShieldMeshComponent || !ShieldMeshComponent->IsVisible())
	{
		return;
	}

	// 너무 많으면 가장 오래된 파동부터 지운다
	while (HitRipples.Num() >= MaxHitRipples)
	{
		if (HitRipples[0].Mesh)
		{
			HitRipples[0].Mesh->DestroyComponent();
		}
		HitRipples.RemoveAt(0);
	}

	FLeeShieldHitRipple& Ripple = HitRipples.AddDefaulted_GetRef();
	Ripple.Mesh = CreateShieldMeshComponent(TEXT("HitRipple"), HitRippleMeshScale);
	Ripple.MID = UMaterialInstanceDynamic::Create(HitRippleMaterial, this);
	Ripple.MID->SetVectorParameterValue(TEXT("Color"), HitRippleColor);
	Ripple.Mesh->SetMaterial(0, Ripple.MID);
	Ripple.LocalDirection = GetActorTransform().InverseTransformVectorNoScale(WorldDirection).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	Ripple.Elapsed = 0.0f;
	UpdateHitRipple(Ripple);

	SetActorTickEnabled(true);
}

// 파동 갱신 — 팩 BP_Hit 커브처럼 반지름은 빠르게 커지다 느려지고(ease-out), 투명도는 1 → 0으로 줄어든다
void ALeeGameplayCue_MagicShield::UpdateHitRipple(FLeeShieldHitRipple& Ripple) const
{
	if (!Ripple.MID)
	{
		return;
	}
	const float T = FMath::Clamp(Ripple.Elapsed / HitRippleDuration, 0.0f, 1.0f);
	const float SizeAlpha = 1.0f - FMath::Pow(1.0f - T, 3.0f);
	const float Opacity = FMath::Pow(1.0f - T, 2.0f);

	// 머티리얼 HitPosition은 월드 좌표 — 쉴드가 움직여도 같은 자리를 따라가도록 매 프레임 다시 계산
	const FVector WorldDirection = GetActorTransform().TransformVectorNoScale(Ripple.LocalDirection);
	Ripple.MID->SetVectorParameterValue(TEXT("HitPosition"), FLinearColor(GetSurfacePoint(WorldDirection)));
	Ripple.MID->SetScalarParameterValue(TEXT("Raidus"), SizeAlpha * CurrentRadius * HitRippleSizeScale);
	Ripple.MID->SetScalarParameterValue(TEXT("Opacity"), Opacity);
}

// 쉴드 구·추가 구·데칼·파동을 숨기고 그 자리에 유리 조각 연출 액터를 남긴다 (이 Cue 액터가 곧 지워져도 조각은 계속 날아간다)
void ALeeGameplayCue_MagicShield::Shatter(const FVector& PushDirection)
{
	if (!ShieldMeshComponent)
	{
		return;
	}

	const FVector Center = ShieldMeshComponent->GetComponentLocation();
	ShieldMeshComponent->SetVisibility(false);
	if (OverlayMeshComponent)
	{
		OverlayMeshComponent->SetVisibility(false);
	}
	if (DecalComponent)
	{
		DecalComponent->SetVisibility(false);
	}
	for (FLeeShieldHitRipple& Ripple : HitRipples)
	{
		if (Ripple.Mesh)
		{
			Ripple.Mesh->SetVisibility(false);
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ALeeShieldShatterActor* ShatterActor = GetWorld()->SpawnActor<ALeeShieldShatterActor>(Center, FRotator::ZeroRotator, SpawnParams))
	{
		ShatterActor->StartShatter(ShardMesh, ShardMaterial, CurrentRadius, ShardCount, ShardLifetime, ShardSpeed, PushDirection.GetSafeNormal());
	}
}

// 피격 번쩍임 시작 — HitFlashDuration 뒤에 끈다
void ALeeGameplayCue_MagicShield::PlayHitFlash()
{
	if (!ShieldMID)
	{
		return;
	}
	ShieldMID->SetScalarParameterValue(HitFlashParameterName, 1.0f);
	GetWorldTimerManager().SetTimer(HitFlashTimer, this, &ThisClass::EndHitFlash, HitFlashDuration, false);
}

// 피격 번쩍임 끝
void ALeeGameplayCue_MagicShield::EndHitFlash()
{
	if (ShieldMID)
	{
		ShieldMID->SetScalarParameterValue(HitFlashParameterName, 0.0f);
	}
}

// 타깃에 붙은 쉴드 Cue 액터 찾기
ALeeGameplayCue_MagicShield* ALeeGameplayCue_MagicShield::FindOnTarget(const AActor* Target)
{
	if (!Target)
	{
		return nullptr;
	}
	TArray<AActor*> Attached;
	Target->GetAttachedActors(Attached);
	for (AActor* Actor : Attached)
	{
		if (ALeeGameplayCue_MagicShield* Shield = Cast<ALeeGameplayCue_MagicShield>(Actor))
		{
			return Shield;
		}
	}
	return nullptr;
}
