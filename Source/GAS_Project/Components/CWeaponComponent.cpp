#include "CWeaponComponent.h"

#include "GAS_Project/Characters/CCharacter.h"
#include "GAS_Project/Item/Weapon/CWeapon.h"
#include "Net/UnrealNetwork.h"


UCWeaponComponent::UCWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheOwnerMesh();

}

void UCWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UCWeaponComponent, CurrentEquippedWeaponTag);
}

void UCWeaponComponent::OnRep_CurrentEquippedWeaponTag()
{
}

ACWeapon* UCWeaponComponent::GetWeaponByTag(FGameplayTag WeaponTag) const
{
	if (ACWeapon* const* FoundWeapon = WeaponMap.Find(WeaponTag))
	{
		return *FoundWeapon;
	}
	return nullptr;
}

class ACWeapon* UCWeaponComponent::GetCurrentWeapon() const
{
	return GetWeaponByTag(CurrentEquippedWeaponTag);
}

void UCWeaponComponent::OnRep_CurrentWeapon(class ACWeapon* OldWeapon)
{
	// 클라이언트에서 무기 변경 시 시각적 업데이트
	UE_LOG(LogTemp, Log, TEXT("OnRep_CurrentWeapon: Old=%s, New=%s"), 
		OldWeapon ? *OldWeapon->GetName() : TEXT("None"),
		CurrentWeapon ? *CurrentWeapon->GetName() : TEXT("None"));

	// UI 업데이트, 사운드 재생 등 클라이언트 측 피드백
	// 예: 무기 교체 사운드, UI 갱신
}

void UCWeaponComponent::AttachWeapon(class ACWeapon* NewWeapon)
{
	// 서버에서만 실행되어야 함 (Ability에서 서버 권한 보장)
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogTemp, Warning, TEXT("AttachWeapon called on non-authority"));
		return;
	}

	if (!NewWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("AttachWeapon: NewWeapon is null"));
		return;
	}

	// 같은 무기면 스킵
	if (NewWeapon == CurrentWeapon)
	{
		return;
	}

	// 기존 무기가 있으면 먼저 해제
	if (CurrentWeapon)
	{
		DetachWeapon();
	}

	// 새 무기 설정
	CurrentWeapon = NewWeapon;
	CurrentWeapon->SetOwner(GetOwner());

	CacheOwnerMesh();

	if (OwnerMesh)
	{
		CurrentWeapon->AttachToComponent(
			OwnerMesh,
			FAttachmentTransformRules::SnapToTargetIncludingScale,
			TEXT("WeaponSocket") // 소켓 이름은 프로젝트에 맞게 수정
		);

		UE_LOG(LogTemp, Log, TEXT("Weapon attached to socket on %s"), *GetOwner()->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("OwnerMesh not found for weapon attachment"));
	}
}

void UCWeaponComponent::DetachWeapon()
{
	// 서버에서만 실행
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogTemp, Warning, TEXT("DetachWeapon called on non-authority"));
		return;
	}

	if (!CurrentWeapon)
	{
		return;
	}

	CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	CurrentWeapon->SetOwner(nullptr);
    
	// 필요시 무기를 파괴하거나 인벤토리로 반환
	// CurrentWeapon->Destroy();
    
	CurrentWeapon = nullptr;

	UE_LOG(LogTemp, Log, TEXT("Weapon detached from %s"), *GetOwner()->GetName());
}

void UCWeaponComponent::CacheOwnerMesh()
{
	if (!OwnerMesh)
	{
		if (ACharacter* OwnerCharacter = Cast<ACCharacter>(GetOwner()))
		{
			OwnerMesh = OwnerCharacter->GetMesh();
		}
	}
}

void UCWeaponComponent::RegisterSpawnedWeapon(struct FGameplayTag InWeaponTag, class ACWeapon* InWeapon, bool bRegister)
{
	// WeaponMap.Emplace(InWeaponTag,InWeapon);
	//
	// if (bRegister)
	// {
	// 	CurrentEquippedWeaponTag = InWeaponTag;
	// }


	if (!InWeaponTag.IsValid() || !InWeapon)
	{
		return;
	}

	if (bRegister)
	{
		WeaponMap.Add(InWeaponTag, InWeapon);
		UE_LOG(LogTemp, Log, TEXT("RegisterSpawnedWeapon: Registered %s"), *InWeaponTag.ToString());
	}
	else
	{
		WeaponMap.Remove(InWeaponTag);
		UE_LOG(LogTemp, Log, TEXT("RegisterSpawnedWeapon: Unregistered %s"), *InWeaponTag.ToString());
	}
}

// ACWeapon* UCWeaponComponent::GetCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const
// {
// 	// if (WeaponMap.Contains(InWeaponTagToGet))
// 	// {
// 	// 	if (ACWeapon* const* FoundWeapon = WeaponMap.Find(InWeaponTagToGet))
// 	// 	{
// 	// 		return *FoundWeapon;
// 	// 	}
// 	// }
// 	//
// 	// return nullptr;
//
// 	
// }

// ACWeapon* UCWeaponComponent::GetCharacterCurrentEquippedWeapon() const
// {
// 	if (!CurrentEquippedWeaponTag.IsValid())
// 	{
// 		return nullptr;
// 	}
//  
// 	return GetCarriedWeaponByTag(CurrentEquippedWeaponTag);
// }
