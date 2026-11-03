#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "LeeGameplayCue_WeaponTelegraph.generated.h"

class UMeshComponent;
class UMaterialInterface;
class UNiagaraSystem;
class UNiagaraComponent;

/**
 * 공격 예고(Telegraph) 무기 발광 + 나이아가라 GameplayCue 액터.
 * - 퍼펙트 가드 전용 공격: GameplayCue.Souls.Attack.PerfectGuardOnly (파란색)
 * - 퍼펙트 회피 전용 공격: GameplayCue.Souls.Attack.Unblockable (빨간색)
 *
 * ANS_WeaponTrace와 동일한 무기 탐색 규약(LeeEquipmentManagerComponent → 폴백 Owner 서브 메시)으로
 * 타겟 액터의 무기 메시를 찾아 WhileActive 동안 OverlayMaterial과 나이아가라 이펙트를 적용하고,
 * OnRemove / EndPlay / Recycle 시 원래 상태로 복원한다.
 */
UCLASS()
class GAS_PROJECT_API ALeeGameplayCue_WeaponTelegraph : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	ALeeGameplayCue_WeaponTelegraph();

	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool Recycle() override;

	/** ANS_WeaponTrace와 동일한 방식으로 액터의 장착 무기 메시를 탐색하는 공용 헬퍼 */
	static UMeshComponent* FindWeaponMeshOnActor(AActor* Owner, FName BaseSocketName = TEXT("WeaponBase"), FName TipSocketName = TEXT("WeaponTip"));

protected:
	/** 무기 메시에 덧씌울 발광 오버레이 머티리얼 (MI_WeaponTelegraph_Blue / Red) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|WeaponTelegraph")
	TObjectPtr<UMaterialInterface> TelegraphOverlayMaterial;

	/** 무기 소켓에 부착할 나이아가라 시스템 (NS_WeaponTelegraph_Blue / Red) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|WeaponTelegraph")
	TObjectPtr<UNiagaraSystem> TelegraphNiagaraSystem;

	/** 나이아가라 이펙트를 부착할 무기 소켓 이름 (없으면 무기 루트에 부착) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|WeaponTelegraph")
	FName AttachSocketName = TEXT("WeaponBase");

	/** 무기 판별용 자루 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|WeaponTelegraph")
	FName WeaponBaseSocket = TEXT("WeaponBase");

	/** 무기 판별용 칼끝 소켓 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|WeaponTelegraph")
	FName WeaponTipSocket = TEXT("WeaponTip");

private:
	void ClearTelegraphEffect();

	TWeakObjectPtr<UMeshComponent> CachedWeaponMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PreviousOverlayMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> SpawnedNiagaraComp;
};

