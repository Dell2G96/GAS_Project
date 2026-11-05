#include "LeeWeaponTelegraphVisuals.h"
#include "GAS_Project/AEquipment/LeeEquipmentMeshSelection.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

void LeeWeaponTelegraphVisuals::Resolve(AActor* Owner, const TArray<FLeeWeaponTelegraphTarget>& Targets, TArray<FLeeResolvedTelegraphTarget>& OutTargets)
{
	OutTargets.Reset();
	TSet<UMeshComponent*> Seen;
	for (const auto& Target : Targets)
	{
		for (UMeshComponent* Mesh : LeeEquipmentMeshSelection::FindMeshes(Owner, Target.EquipmentAttachSocket))
		{
			if (Seen.Contains(Mesh) || (!Target.EffectAttachSocket.IsNone() && !Mesh->DoesSocketExist(Target.EffectAttachSocket))) continue;
			Seen.Add(Mesh);
			OutTargets.Add({Mesh, Target.EffectAttachSocket});
		}
	}
}

void LeeWeaponTelegraphVisuals::Apply(const TArray<FLeeResolvedTelegraphTarget>& Targets, UMaterialInterface* Overlay, UNiagaraSystem* Niagara,
	TArray<FLeeWeaponTelegraphVisualState>& OutStates, UMeshComponent* SkipOverlayMesh)
{
	Clear(OutStates);
	for (const auto& Target : Targets)
	{
		UMeshComponent* Mesh = Target.Mesh.Get();
		if (!Mesh) continue;
		auto& State = OutStates.AddDefaulted_GetRef();
		State.Mesh = Mesh;
		if (Overlay && Mesh != SkipOverlayMesh)
		{
			State.PreviousOverlay.Reset(Mesh->GetOverlayMaterial());
			Mesh->SetOverlayMaterial(Overlay);
			State.bAppliedOverlay = true;
		}
		if (Niagara)
			State.Niagara = UNiagaraFunctionLibrary::SpawnSystemAttached(Niagara, Mesh, Target.Socket, FVector::ZeroVector,
				FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
	}
}

void LeeWeaponTelegraphVisuals::Clear(TArray<FLeeWeaponTelegraphVisualState>& States)
{
	for (auto& State : States)
	{
		if (auto* Mesh = State.Mesh.Get(); Mesh && State.bAppliedOverlay) Mesh->SetOverlayMaterial(State.PreviousOverlay.Get());
		if (auto* Niagara = State.Niagara.Get()) { Niagara->DeactivateImmediate(); Niagara->DestroyComponent(); }
	}
	States.Reset();
}
