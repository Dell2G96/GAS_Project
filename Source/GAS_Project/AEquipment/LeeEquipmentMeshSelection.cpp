#include "LeeEquipmentMeshSelection.h"
#include "LeeEquipmentManagerComponent.h"
#include "LeeEquipmentInstance.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"

namespace
{
	bool MatchesAttachment(const USceneComponent* Component, const AActor* Owner, FName Socket)
	{
		for (const USceneComponent* Current = Component; Current; Current = Current->GetAttachParent())
		{
			const USceneComponent* Parent = Current->GetAttachParent();
			if (Parent && Parent->GetOwner() == Owner && Current->GetAttachSocketName() == Socket) return true;
		}
		return false;
	}
}

// Persona는 미리보기 장비를 WorldSettings 소유로 만들므로 실제 부착 계층을 읽는다.
TArray<UMeshComponent*> LeeEquipmentMeshSelection::FindPreviewMeshes(const AActor* Owner)
{
	TArray<UMeshComponent*> Result;
#if WITH_EDITOR
	if (!Owner || !Owner->GetWorld() || Owner->GetWorld()->WorldType != EWorldType::EditorPreview)
	{
		return Result;
	}

	TArray<UMeshComponent*> OwnerMeshes;
	Owner->GetComponents(OwnerMeshes);
	for (const UMeshComponent* Parent : OwnerMeshes)
	{
		for (USceneComponent* Child : Parent->GetAttachChildren())
		{
			if (UMeshComponent* Mesh = Cast<UMeshComponent>(Child); IsValid(Mesh))
			{
				Result.AddUnique(Mesh);
			}
		}
	}
#endif
	return Result;
}

// 장비 액터와 캐릭터 메시, 에디터 미리보기 장비에서 지정된 부착 소켓을 찾는다.
TArray<UMeshComponent*> LeeEquipmentMeshSelection::FindMeshes(AActor* Owner, FName EquipmentAttachSocket)
{
	TArray<UMeshComponent*> Result;
	if (!Owner || EquipmentAttachSocket.IsNone()) return Result;
	auto Gather = [&](AActor* Actor)
	{
		if (!IsValid(Actor)) return;
		TArray<UMeshComponent*> Meshes;
		Actor->GetComponents(Meshes);
		for (UMeshComponent* Mesh : Meshes)
			if (IsValid(Mesh) && MatchesAttachment(Mesh, Owner, EquipmentAttachSocket)) Result.AddUnique(Mesh);
	};
	if (const auto* Manager = Owner->FindComponentByClass<ULeeEquipmentManagerComponent>())
		for (const auto* Instance : Manager->GetEquipmentInstancesOfType(ULeeEquipmentInstance::StaticClass()))
			if (IsValid(Instance)) for (AActor* Actor : Instance->GetSpawnedActors()) Gather(Actor);
	Gather(Owner); // BP에서 직접 붙인 장비 메시도 지원한다.
	for (UMeshComponent* Mesh : FindPreviewMeshes(Owner))
	{
		if (MatchesAttachment(Mesh, Owner, EquipmentAttachSocket))
		{
			Result.AddUnique(Mesh);
		}
	}
	return Result;
}

// 26.11.25 추가 - 장비 액터 → 캐릭터에 직접 붙은 메시 순서로, 손잡이·칼끝 소켓을 모두 가진 첫 메시의 칼날 선분을 구한다 (방패 등은 소켓이 없어 자동 제외)
bool LeeEquipmentMeshSelection::FindWeaponBlade(AActor* Owner, FName BaseSocket, FName TipSocket, FVector& OutBase, FVector& OutTip)
{
	if (!Owner || BaseSocket.IsNone() || TipSocket.IsNone()) return false;

	TArray<AActor*> Candidates;
	if (const auto* Manager = Owner->FindComponentByClass<ULeeEquipmentManagerComponent>())
		for (const auto* Instance : Manager->GetEquipmentInstancesOfType(ULeeEquipmentInstance::StaticClass()))
			if (IsValid(Instance)) Candidates.Append(Instance->GetSpawnedActors());
	Candidates.Add(Owner);

	for (AActor* Actor : Candidates)
	{
		if (!IsValid(Actor)) continue;
		TArray<UMeshComponent*> Meshes;
		Actor->GetComponents(Meshes);
		for (const UMeshComponent* Mesh : Meshes)
		{
			if (IsValid(Mesh) && Mesh->DoesSocketExist(BaseSocket) && Mesh->DoesSocketExist(TipSocket))
			{
				OutBase = Mesh->GetSocketLocation(BaseSocket);
				OutTip = Mesh->GetSocketLocation(TipSocket);
				return true;
			}
		}
	}
	return false;
}
