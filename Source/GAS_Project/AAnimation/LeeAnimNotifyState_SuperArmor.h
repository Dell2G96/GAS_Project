// 26.11.11 신규 - 슈퍼아머 구간 AnimNotifyState

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "LeeAnimNotifyState_SuperArmor.generated.h"

/** 슈퍼아머 단계 — 어떤 피격까지 버티는지 */
UENUM(BlueprintType)
enum class ELeeSuperArmorLevel : uint8
{
	/** 데미지 받음, 일반 경직만 무시 (강공격에는 경직) */
	Light    UMETA(DisplayName = "Light (일반 경직 무시)"),

	/** 데미지 받음, 일반 + 강공격 경직 모두 무시 */
	Heavy    UMETA(DisplayName = "Heavy (강경직까지 무시)"),

	/** 데미지 0, 모든 경직 무시 (가드불가 공격도 못 뚫음). 히트 이펙트는 나온다 */
	NoDamage UMETA(DisplayName = "No Damage (데미지 0)")
};

/**
 * 몽타주 구간 동안 피격자 ASC에 슈퍼아머 태그(Souls.Status.SuperArmor.*)를 붙이는 노티파이 스테이트.
 * 판정은 서버에서만 일어나므로 태그도 서버에서만 붙이고 뗀다. 플레이어·적 공용.
 */
UCLASS(DisplayName = "Lee Super Armor")
class GAS_PROJECT_API ULeeAnimNotifyState_SuperArmor : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	ULeeAnimNotifyState_SuperArmor();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	/** 이 구간에 걸 슈퍼아머 단계 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|SuperArmor")
	ELeeSuperArmorLevel ArmorLevel = ELeeSuperArmorLevel::Light;

private:
	/** ArmorLevel → 붙일 태그 */
	FGameplayTag GetArmorTag() const;
};
