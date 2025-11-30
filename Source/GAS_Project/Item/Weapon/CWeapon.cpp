// CWeapon.cpp
#include "CWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"

ACWeapon::ACWeapon()
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	SheathMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SheathMesh"));
	SheathMesh->SetupAttachment(GetRootComponent());

	WeaponCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollisionBox"));
	WeaponCollisionBox->SetupAttachment(GetRootComponent());
	WeaponCollisionBox->SetBoxExtent(FVector(20.f));
	WeaponCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bReplicates = true;
	SetReplicateMovement(true);
	bNetUseOwnerRelevancy = true;
}

void ACWeapon::SetupWeaponAttach(USceneComponent* InParent, FName InWeaponSocketName, FName InSheathSocketName)
{
	// 서버에서만 Replicated 데이터 세팅
	if (!HasAuthority())
	{
		return;
	}

	AttachSplite.Parent = InParent;
	AttachSplite.WeaponSocketName = InWeaponSocketName;
	AttachSplite.SheathSocketName = InSheathSocketName;

	// 서버 로컬에서도 바로 적용
	ApplyAttachSplite();
}

void ACWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACWeapon, AttachSplite);

}

void ACWeapon::OnRep_AttachSplite()
{
	// 클라에서 Attach 정보가 갱신될 때마다 실행
	ApplyAttachSplite();
}

void ACWeapon::ApplyAttachSplite()
{
	// 유효성 체크
	if (!WeaponMesh || !SheathMesh) return;

	USceneComponent* ParentComp = AttachSplite.Parent;
	if (!ParentComp)
	{
		// 부모가 없으면 루트에라도 붙이기 (안전망)
		ParentComp = GetRootComponent();
	}

	if (!ParentComp) return;

	
	const FAttachmentTransformRules AttachRules(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepRelative,
		true    // bWeldSimulatedBodies
	);

	// 칼
	if (AttachSplite.WeaponSocketName != NAME_None)
	{
		WeaponMesh->AttachToComponent(
			ParentComp,
			AttachRules,
			AttachSplite.WeaponSocketName
		);
	}
	else
	{
		// 소켓 이름이 없으면 부모에 그냥 붙이기
		WeaponMesh->AttachToComponent(
			ParentComp,
			AttachRules
		);
	}

	// 칼집
	if (AttachSplite.SheathSocketName != NAME_None)
	{
		SheathMesh->AttachToComponent(
			ParentComp,
			AttachRules,
			AttachSplite.SheathSocketName
		);
	}
	else
	{
		SheathMesh->AttachToComponent(
			ParentComp,
			AttachRules
		);
	}
}

