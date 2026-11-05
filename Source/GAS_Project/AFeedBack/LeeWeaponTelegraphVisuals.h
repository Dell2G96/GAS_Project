#pragma once
#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"
#include "LeeWeaponTelegraphVisuals.generated.h"

class UMeshComponent;
class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FLeeWeaponTelegraphTarget
{
	GENERATED_BODY()
	/** WID에서 장비 액터를 캐릭터에 부착한 소켓. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target")
	FName EquipmentAttachSocket;
	/** 선택한 장비 메시 내부의 이펙트 위치. None이면 해당 메시 루트. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Target")
	FName EffectAttachSocket = TEXT("WeaponBase");
};

struct FLeeResolvedTelegraphTarget
{
	TWeakObjectPtr<UMeshComponent> Mesh;
	FName Socket;
};

struct FLeeWeaponTelegraphVisualState
{
	TWeakObjectPtr<UMeshComponent> Mesh;
	TStrongObjectPtr<UMaterialInterface> PreviousOverlay;
	TWeakObjectPtr<UNiagaraComponent> Niagara;
	bool bAppliedOverlay = false;
};

namespace LeeWeaponTelegraphVisuals
{
	GAS_PROJECT_API void Resolve(AActor* Owner, const TArray<FLeeWeaponTelegraphTarget>& Targets, TArray<FLeeResolvedTelegraphTarget>& OutTargets);
	GAS_PROJECT_API void Apply(const TArray<FLeeResolvedTelegraphTarget>& Targets, UMaterialInterface* Overlay, UNiagaraSystem* Niagara,
		TArray<FLeeWeaponTelegraphVisualState>& OutStates, UMeshComponent* SkipOverlayMesh = nullptr);
	GAS_PROJECT_API void Clear(TArray<FLeeWeaponTelegraphVisualState>& States);
}
