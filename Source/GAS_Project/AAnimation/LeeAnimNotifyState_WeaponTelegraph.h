#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "GAS_Project/AFeedBack/LeeWeaponTelegraphVisuals.h"
#include "LeeAnimNotifyState_WeaponTelegraph.generated.h"

class UMeshComponent;
class UMaterialInterface;
class UNiagaraSystem;
class UNiagaraComponent;

/**
 * 공격 예고 구간(몇 초) 동안 Enemy 무기에 파란색(퍼펙트 가드 전용) 또는 빨간색(퍼펙트 회피 전용)
 * 불빛(Overlay Material + Niagara)을 켜고, 구간이 끝나면 자동으로 끄는 AnimNotifyState.
 *
 * - CueTag(기본: GameplayCue.Souls.Attack.PerfectGuardOnly | GameplayCue.Souls.Attack.Unblockable)가
 *   지정되어 있으면 서버에서 ASC->AddGameplayCue / RemoveGameplayCue를 호출해 GameplayCue도 함께 트리거한다.
 * - 동시에 OverrideOverlayMaterial / OverrideNiagaraSystem이 지정되어 있으면(또는 기본 에셋 자동 로드)
 *   애니메이션 에디터 프리뷰 창에서도 즉시 무기 발광과 나이아가라 이펙트를 눈으로 확인하며 구간을 조절할 수 있다.
 * - [타격별 공격 속성] 구간 동안 공격자 ASC에 DamageType Loose Tag를 걸어 GA_AttackMelee가 히트 순간 판정에 반영한다.
 *   발광은 GlowDuration초 뒤 먼저 꺼지고, 속성 태그는 구간 끝(NotifyEnd)까지 유지된다 → 노티파이를 윈드업~Trace 끝까지 배치.
 */
UENUM(BlueprintType)
enum class ELeeWeaponTelegraphType : uint8
{
	/** 퍼펙트 가드로만 방어 가능한 공격 (파란색) */
	PerfectGuardOnly UMETA(DisplayName = "Perfect Guard Only (Blue)"),

	/** 퍼펙트 회피로만 회피 가능한 가드불가 공격 (빨간색) */
	Unblockable      UMETA(DisplayName = "Perfect Dodge Only / Unblockable (Red)"),

	/** 사용자 직접 지정 */
	Custom           UMETA(DisplayName = "Custom")
};

UCLASS(DisplayName = "Lee Weapon Telegraph (Perfect Guard/Dodge Glow)")
class GAS_PROJECT_API ULeeAnimNotifyState_WeaponTelegraph : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	ULeeAnimNotifyState_WeaponTelegraph();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	const TArray<FLeeWeaponTelegraphTarget>& GetEquipmentTargets() const { return EquipmentTargets; }
	FName GetEffectAttachSocket() const { return AttachSocketName; }
	UMaterialInterface* GetTelegraphOverlay() const { return ResolveOverlayMaterial(); }
	UNiagaraSystem* GetTelegraphNiagara() const { return ResolveNiagaraSystem(); }

protected:
	/** 비어 있으면 기존 첫 무기 탐색. 명시하면 해당 캐릭터 부착 소켓의 장비만 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lee|WeaponTelegraph|Targets")
	TArray<FLeeWeaponTelegraphTarget> EquipmentTargets;
	/** 예고 타입 선택 — 파란색(퍼펙트 가드 전용) / 빨간색(퍼펙트 회피 전용) 프리셋 자동 적용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|WeaponTelegraph")
	ELeeWeaponTelegraphType TelegraphType = ELeeWeaponTelegraphType::PerfectGuardOnly;

	/** Custom 모드이거나 기본 프리셋 대신 덮어쓸 GameplayCue 태그 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|WeaponTelegraph", meta = (Categories = "GameplayCue"))
	FGameplayTag CustomCueTag;

	/** 무기 표면에 덧씌울 발광 오버레이 머티리얼 (비워두면 TelegraphType에 맞춰 MI_WeaponTelegraph_Blue/Red 자동 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|WeaponTelegraph")
	TObjectPtr<UMaterialInterface> OverrideOverlayMaterial;

	/** 무기 소켓에 부착할 나이아가라 이펙트 (비워두면 TelegraphType에 맞춰 NS_WeaponTelegraph_Blue/Red 자동 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|WeaponTelegraph")
	TObjectPtr<UNiagaraSystem> OverrideNiagaraSystem;

	/** 나이아가라 이펙트를 붙일 무기 소켓 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|WeaponTelegraph")
	FName AttachSocketName = TEXT("WeaponBase");

	/** 구간 시작 후 발광을 유지할 시간(초). 0이면 구간 전체 동안 발광. 속성 태그는 이 값과 무관하게 구간 끝까지 유지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|WeaponTelegraph", meta = (ClampMin = "0.0"))
	float GlowDuration = 0.0f;

private:
	struct FTelegraphRuntimeData
	{
		TArray<FLeeWeaponTelegraphVisualState> Visuals;
		FGameplayTag ActiveCueTag;

		/** 공격자 ASC에 건 타격 속성 Loose Tag (서버 전용) */
		FGameplayTag ActiveDamageTypeTag;
		/** 구간 시작 후 경과 시간 — GlowDuration 판정용 */
		float ElapsedTime = 0.0f;
		/** 발광(오버레이/나이아가라/Cue)이 아직 켜져 있는지 */
		bool bGlowActive = false;
	};

	TMap<TWeakObjectPtr<USkeletalMeshComponent>, FTelegraphRuntimeData> RuntimeMap;

	/** 발광만 끈다 (Cue 제거 + 오버레이 복원 + 나이아가라 제거). 속성 태그는 건드리지 않음 */
	void StopGlow(USkeletalMeshComponent* MeshComp, FTelegraphRuntimeData& Data) const;

	/** TelegraphType → 데미지 판정용 DamageType 태그 (Custom은 판정 없음 → 빈 태그) */
	FGameplayTag ResolveDamageTypeTag() const;

	FGameplayTag ResolveCueTag() const;
	UMaterialInterface* ResolveOverlayMaterial() const;
	UNiagaraSystem* ResolveNiagaraSystem() const;
};

