// CWeapon.cpp
#include "CWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"

ACWeapon::ACWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// 1) 루트 먼저 만들기
	Root = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("Root"));
	SetRootComponent(Root);

	// 2) 메쉬 생성 + 루트에 부착
	WeaponMesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(Root);

	// 3) 콜리전 박스는 ObjectInitializer로 생성해야 함 (포인터 역참조 금지)
	WeaponCollisionBox = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, TEXT("WeaponCollisionBox"));
	WeaponCollisionBox->SetupAttachment(WeaponMesh); // 또는 Root
	WeaponCollisionBox->SetBoxExtent(FVector(20.f));
	WeaponCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
