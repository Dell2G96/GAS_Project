// 26.11.08 신규 - 마법 쉴드 표시 GameplayCue (GameplayCue.Souls.Shield.Active)

#include "LeeGameplayCue_MagicShield.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AFeedBack/LeeShieldShatterActor.h"

// 생성자 — Cue 태그 지정, 제거 시 자동 파괴, 기본 메시는 엔진 구
ALeeGameplayCue_MagicShield::ALeeGameplayCue_MagicShield()
{
	GameplayCueTag = MyTags::Souls::GameplayCue_Shield_Active;
	bAutoDestroyOnRemove = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		ShieldMesh = SphereMesh.Object;
	}

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
}

// 쉴드 켜짐 — 타깃에 붙어 구 메시를 만들고 금 0으로 시작
bool ALeeGameplayCue_MagicShield::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (!MyTarget || !ShieldMesh)
	{
		return false;
	}

	AttachToActor(MyTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	ShieldMeshComponent = NewObject<UStaticMeshComponent>(this, TEXT("ShieldMesh"));
	ShieldMeshComponent->SetStaticMesh(ShieldMesh);
	ShieldMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldMeshComponent->SetCastShadow(false);
	ShieldMeshComponent->SetupAttachment(GetRootComponent());
	ShieldMeshComponent->RegisterComponent();

	// 엔진 구는 반지름 50cm — 캐릭터 캡슐을 덮을 만큼 키운다
	float Radius = ShieldRadius;
	if (const ACharacter* Character = Cast<ACharacter>(MyTarget))
	{
		Radius = FMath::Max(Radius, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 1.1f);
	}
	ShieldMeshComponent->SetRelativeScale3D(FVector(Radius / 50.0f));
	CurrentRadius = Radius;

	if (ShieldMaterial)
	{
		ShieldMID = UMaterialInstanceDynamic::Create(ShieldMaterial, this);
		ShieldMeshComponent->SetMaterial(0, ShieldMID);
	}

	// 금 없이 시작
	NumCrackImpacts = 0;
	NextCrackSlot = 0;
	CrackSpread = 0.0f;
	UpdateCrackParameters();
	return true;
}

// 쉴드 꺼짐 — 메시 제거 (액터는 bAutoDestroyOnRemove로 정리된다)
bool ALeeGameplayCue_MagicShield::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	GetWorldTimerManager().ClearTimer(HitFlashTimer);
	if (ShieldMeshComponent)
	{
		ShieldMeshComponent->DestroyComponent();
		ShieldMeshComponent = nullptr;
	}
	ShieldMID = nullptr;
	return true;
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

// 쉴드 구를 숨기고 그 자리에 유리 조각 연출 액터를 남긴다 (이 Cue 액터가 곧 지워져도 조각은 계속 날아간다)
void ALeeGameplayCue_MagicShield::Shatter(const FVector& PushDirection)
{
	if (!ShieldMeshComponent)
	{
		return;
	}

	const FVector Center = ShieldMeshComponent->GetComponentLocation();
	ShieldMeshComponent->SetVisibility(false);

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
