#include "LeeGameplayCue_WeaponTelegraph.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GAS_Project/AEquipment/LeeEquipmentInstance.h"
#include "GAS_Project/AEquipment/LeeEquipmentManagerComponent.h"

ALeeGameplayCue_WeaponTelegraph::ALeeGameplayCue_WeaponTelegraph()
{
	PrimaryActorTick.bCanEverTick = false;
	bAutoDestroyOnRemove = true;
}

bool ALeeGameplayCue_WeaponTelegraph::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (GetNetMode() == NM_DedicatedServer || !MyTarget)
	{
		return false;
	}

	ClearTelegraphEffect();

	UMeshComponent* WeaponMesh = FindWeaponMeshOnActor(MyTarget, WeaponBaseSocket, WeaponTipSocket);
	if (!WeaponMesh)
	{
		return false;
	}

	CachedWeaponMesh = WeaponMesh;

	// 1. 발광 오버레이 머티리얼 적용
	if (TelegraphOverlayMaterial)
	{
		PreviousOverlayMaterial = WeaponMesh->GetOverlayMaterial();
		WeaponMesh->SetOverlayMaterial(TelegraphOverlayMaterial);
	}

	// 2. 나이아가라 이펙트 부착
	if (TelegraphNiagaraSystem)
	{
		const FName SocketToUse = WeaponMesh->DoesSocketExist(AttachSocketName) ? AttachSocketName : NAME_None;
		SpawnedNiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TelegraphNiagaraSystem,
			WeaponMesh,
			SocketToUse,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true);
	}

	return true;
}

bool ALeeGameplayCue_WeaponTelegraph::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	ClearTelegraphEffect();
	return true;
}

void ALeeGameplayCue_WeaponTelegraph::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearTelegraphEffect();
	Super::EndPlay(EndPlayReason);
}

bool ALeeGameplayCue_WeaponTelegraph::Recycle()
{
	ClearTelegraphEffect();
	return Super::Recycle();
}

void ALeeGameplayCue_WeaponTelegraph::ClearTelegraphEffect()
{
	if (UMeshComponent* WeaponMesh = CachedWeaponMesh.Get())
	{
		if (TelegraphOverlayMaterial)
		{
			WeaponMesh->SetOverlayMaterial(PreviousOverlayMaterial);
		}
	}

	if (SpawnedNiagaraComp)
	{
		SpawnedNiagaraComp->DeactivateImmediate();
		SpawnedNiagaraComp->DestroyComponent();
		SpawnedNiagaraComp = nullptr;
	}

	PreviousOverlayMaterial = nullptr;
	CachedWeaponMesh.Reset();
}

UMeshComponent* ALeeGameplayCue_WeaponTelegraph::FindWeaponMeshOnActor(AActor* Owner, FName BaseSocketName, FName TipSocketName)
{
	if (!Owner)
	{
		return nullptr;
	}

	const ACharacter* OwnerChar = Cast<ACharacter>(Owner);
	const USkeletalMeshComponent* BodyMesh = OwnerChar ? OwnerChar->GetMesh() : nullptr;

	// 1. LeeEquipmentManagerComponent의 스폰된 무기 액터에서 두 소켓을 가진 메시 탐색
	if (ULeeEquipmentManagerComponent* EquipMgr = Owner->FindComponentByClass<ULeeEquipmentManagerComponent>())
	{
		const TArray<ULeeEquipmentInstance*> Instances = EquipMgr->GetEquipmentInstancesOfType(ULeeEquipmentInstance::StaticClass());
		for (ULeeEquipmentInstance* Inst : Instances)
		{
			if (!Inst) continue;

			for (AActor* WeaponActor : Inst->GetSpawnedActors())
			{
				if (!WeaponActor) continue;

				TArray<UMeshComponent*> Meshes;
				WeaponActor->GetComponents<UMeshComponent>(Meshes);
				for (UMeshComponent* M : Meshes)
				{
					if (M && M->DoesSocketExist(BaseSocketName) && M->DoesSocketExist(TipSocketName))
					{
						return M;
					}
				}
			}
		}

		// 소켓 조건이 없는 무기 메시라도 있으면 첫 번째 메시 반환
		for (ULeeEquipmentInstance* Inst : Instances)
		{
			if (!Inst) continue;

			for (AActor* WeaponActor : Inst->GetSpawnedActors())
			{
				if (!WeaponActor) continue;

				if (UMeshComponent* AnyMesh = WeaponActor->FindComponentByClass<UMeshComponent>())
				{
					return AnyMesh;
				}
			}
		}
	}

	// 2. [폴백] 캐릭터 액터에 직접 부착된 서브 메시 컴포넌트에서 탐색
	TArray<UMeshComponent*> OwnerMeshes;
	Owner->GetComponents<UMeshComponent>(OwnerMeshes);
	for (UMeshComponent* M : OwnerMeshes)
	{
		if (!M || M == BodyMesh) continue;

		if (M->DoesSocketExist(BaseSocketName) && M->DoesSocketExist(TipSocketName))
		{
			return M;
		}
	}

	for (UMeshComponent* M : OwnerMeshes)
	{
		if (M && M != BodyMesh)
		{
			return M;
		}
	}

	return nullptr;
}

