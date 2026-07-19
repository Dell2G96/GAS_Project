#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "LeeGameplayCue_PerfectDodge.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPoseableMeshComponent;

/**
 * 퍼펙트 회피 잔상 GameplayCue (태그: GameplayCue.Souls.Dodge.Perfect).
 * 클라이언트 전용 연출 — 서버 판정(무적/데미지 무효)과 완전 분리, Cue가 실행 안 돼도 게임플레이 영향 없음.
 *
 * 동작 (Poseable Mesh 스냅샷 방식):
 *  OnExecute → SampleInterval 간격으로 타겟 SkeletalMesh 포즈를 PoseableMeshComponent로 복사해
 *  월드에 남기고(최대 MaxCount개), 각 잔상은 AfterimageLifetime 동안 페이드아웃 후 파괴.
 *  페이드는 AfterimageMaterial의 스칼라 파라미터(OpacityParamName)로 제어한다.
 */
UCLASS()
class GAS_PROJECT_API ALeeGameplayCue_PerfectDodge : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	ALeeGameplayCue_PerfectDodge();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	
private:
	/** 타겟 메시 포즈를 복사한 잔상 1개 스폰 */
	void SpawnAfterimageSample();

	/** 모든 잔상의 페이드 갱신 + 수명 만료 잔상 파괴 */
	void UpdateAfterimageFade();

	/** 잔상/타이머 전부 정리 (재실행/EndPlay 대비) */
	void ClearAfterimages();

	/** 스폰된 잔상 컴포넌트와 스폰 시각 (인덱스 병렬 배열). 슬롯별 MID는 Sample->GetMaterial()로 필요 시 다시 조회 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPoseableMeshComponent>> AfterimageSamples;

	TArray<double> AfterimageSpawnTimes;

	/** 포즈 원본 메시 (Cue 타겟 캐릭터의 SkeletalMesh) */
	TWeakObjectPtr<USkeletalMeshComponent> SourceMesh;

	FTimerHandle SampleTimerHandle;
	FTimerHandle FadeTimerHandle;
	int32 SpawnedSampleCount = 0;

protected:
	/** 잔상 샘플링 간격 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Afterimage", meta = (ClampMin = "0.01"))
	float AfterimageSampleInterval = 0.025f;

	/** 잔상 개별 수명 (초) — 이 시간 동안 페이드아웃 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Afterimage", meta = (ClampMin = "0.01"))
	float AfterimageLifetime = 0.25f;

	/** 최대 잔상 개수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Afterimage", meta = (ClampMin = "1"))
	int32 AfterimageMaxCount = 8;

	/** 잔상 틴트 컬러 (기본: 연회색) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Afterimage")
	FLinearColor AfterimageTint = FLinearColor(0.8f, 0.8f, 0.85f, 1.0f);

	/** 잔상에 씌울 반투명 머티리얼(폴백). AfterimageMaterialMap에 없는 슬롯에 사용. 스칼라(OpacityParamName)/벡터(TintParamName) 파라미터 필요 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Afterimage")
	TObjectPtr<UMaterialInterface> AfterimageMaterial;

	/** 원본 슬롯 머티리얼 → 반투명 잔상 머티리얼 매핑. 슬롯별로 다른 반투명 버전을 쓸 때 사용(멀티 슬롯 캐릭터용) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Afterimage")
	TMap<TObjectPtr<UMaterialInterface>, TObjectPtr<UMaterialInterface>> AfterimageMaterialMap;

	/** 머티리얼의 불투명도 스칼라 파라미터 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Afterimage")
	FName OpacityParamName = TEXT("Opacity");

	/** 머티리얼의 틴트 벡터 파라미터 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Afterimage")
	FName TintParamName = TEXT("Tint");

};
