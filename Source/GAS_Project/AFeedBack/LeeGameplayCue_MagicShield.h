// 26.11.08 신규 - 마법 쉴드 표시 GameplayCue (GameplayCue.Souls.Shield.Active)

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "LeeGameplayCue_MagicShield.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * 마법 쉴드가 켜진 동안 Enemy를 감싸는 구 메시를 보여 주는 Actor Cue.
 * 맞은 자리마다 머티리얼 Hit0~3에 금(균열 텍스처 투영)을 새기고, HitFlash로 피격 번쩍임을 표현한다.
 * 깨질 때는 구를 숨기고 유리 조각 연출(ALeeShieldShatterActor)을 스폰한다.
 * 호출은 피격 Cue(ULeeGameplayCueNotify_ShieldHit)가 AddCrackImpact/PlayHitFlash/Shatter로 한다.
 * BP는 이 클래스를 부모로 만들고 메시·머티리얼만 지정한다.
 */
UCLASS()
class GAS_PROJECT_API ALeeGameplayCue_MagicShield : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	ALeeGameplayCue_MagicShield();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

	/** 26.11.08 수정 - 맞은 방향(월드)에 금을 새기고, 모든 금을 CrackAmount(0 = 깨끗함, 1 = 곧 깨짐)만큼 퍼뜨린다.
	 *  금 자리는 최대 MaxCrackImpacts곳, 기존 자리와 가까우면 그 자리를 다시 쓴다 */
	void AddCrackImpact(const FVector& WorldDirection, float CrackAmount);

	/** 피격 순간 쉴드를 잠깐 밝게 번쩍인다 */
	void PlayHitFlash();

	/** 26.11.08 추가 - 쉴드 구를 숨기고 유리 조각 연출 액터를 스폰한다 (PushDirection = 조각이 더 밀려 갈 방향) */
	void Shatter(const FVector& PushDirection);

	/** 26.11.08 추가 - 맞은 방향(월드)에 해당하는 쉴드 표면 위치 */
	FVector GetSurfacePoint(const FVector& WorldDirection) const;

	/** 타깃에 붙어 있는 쉴드 Cue 액터를 찾는다 (없으면 nullptr) */
	static ALeeGameplayCue_MagicShield* FindOnTarget(const AActor* Target);

protected:
	/** 쉴드 모양 메시 (기본: 엔진 구, 반지름 50cm 기준) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield")
	TObjectPtr<UStaticMesh> ShieldMesh;

	/** 쉴드 머티리얼 (M_MagicShield 계열, CrackAmount·HitFlash 스칼라 파라미터 필요) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield")
	TObjectPtr<UMaterialInterface> ShieldMaterial;

	/** 쉴드 반지름(cm). 캡슐 반 높이가 더 크면 그 값을 쓴다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield", meta = (ClampMin = "10.0"))
	float ShieldRadius = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield")
	FName HitFlashParameterName = TEXT("HitFlash");

	/** 번쩍임 유지 시간(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield", meta = (ClampMin = "0.01"))
	float HitFlashDuration = 0.15f;

	/** 26.11.08 추가 - 첫 피격 때 금이 퍼진 정도(0~1). 이후 CrackAmount에 비례해 1까지 퍼진다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield|Crack", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinCrackSpread = 0.3f;

	/** 26.11.08 추가 - 이 각도(도) 안에서 다시 맞으면 새 금을 만들지 않고 기존 금 자리를 쓴다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield|Crack", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float CrackMergeAngle = 25.0f;

	/** 26.11.08 추가 - 깨질 때 흩어지는 유리 조각 메시 (+X가 바깥을 보는 납작한 조각, SM_ShieldShard) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield|Shatter")
	TObjectPtr<UStaticMesh> ShardMesh;

	/** 26.11.08 추가 - 유리 조각 머티리얼 (MI_MagicShield_Shard) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield|Shatter")
	TObjectPtr<UMaterialInterface> ShardMaterial;

	/** 26.11.08 추가 - 조각 개수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield|Shatter", meta = (ClampMin = "1", ClampMax = "200"))
	int32 ShardCount = 48;

	/** 26.11.08 추가 - 조각이 사라질 때까지 시간(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield|Shatter", meta = (ClampMin = "0.1"))
	float ShardLifetime = 1.0f;

	/** 26.11.08 추가 - 조각이 튀어 나가는 최대 속도(cm/s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield|Shatter", meta = (ClampMin = "0.0"))
	float ShardSpeed = 450.0f;

private:
	/** 번쩍임을 끈다 */
	void EndHitFlash();

	/** 금 자리 4곳을 머티리얼 Hit0~Hit3(xyz = 쉴드 기준 방향, w = 퍼진 정도)에 넣는다 */
	void UpdateCrackParameters();

	/** 머티리얼 Hit0~Hit3와 맞춘 금 자리 수 */
	static constexpr int32 MaxCrackImpacts = 4;

	/** 금 자리 방향 (쉴드 메시 기준) */
	FVector CrackDirections[MaxCrackImpacts];

	/** 지금까지 만든 금 자리 수 / 다음에 덮어쓸 자리 */
	int32 NumCrackImpacts = 0;
	int32 NextCrackSlot = 0;

	/** 모든 금이 퍼진 정도 (0 = 안 보임) */
	float CrackSpread = 0.0f;

	/** 실제 쉴드 반지름(cm) — 표면 위치·조각 배치용 */
	float CurrentRadius = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ShieldMeshComponent;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ShieldMID;

	FTimerHandle HitFlashTimer;
};
