// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LeeEnemySensingComponent.generated.h"

class UAbilitySystemComponent;

/**
 * 경량 벡터 기반 인식 컴포넌트 (정식 AI Perception 미사용).
 * Enemy Blueprint에 부착한다. 판정은 서버 전용.
 *
 * 주기(SenseInterval)마다 모든 플레이어 폰에 대해
 *  ① 거리 ≤ SightRadius ② 전방 시야각(SightAngleDeg) 이내 ③ (옵션) LineTrace 차폐 없음
 * 을 검사해 하나라도 인식되면 Status.Unaware 태그를 즉시 제거한다.
 * → 다음 판정 주기에 LeeFinisherTargetComponent가 암살 프롬프트를 숨긴다.
 *
 * LoseSightDelay초 동안 아무도 인식하지 못하면 Status.Unaware를 재부여한다 (전투 이탈).
 * 태그는 서버 로컬 + 리플리케이트 양쪽에 반영해 클라이언트의 UI 판정에서도 보이게 한다.
 */
UCLASS(Blueprintable, ClassGroup = (Lee), meta = (BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeEnemySensingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeeEnemySensingComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 시야 거리 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Sensing", meta = (ClampMin = "0.0"))
	float SightRadius = 1200.0f;

	/** 전방 시야각 (도, 원뿔 전체 각) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Sensing", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float SightAngleDeg = 90.0f;

	/** 시야 차폐(LineTrace) 검사 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Sensing")
	bool bCheckLineOfSight = true;

	/** 인식 판정 주기 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Sensing", meta = (ClampMin = "0.05"))
	float SenseInterval = 0.2f;

	/** 시야에서 사라진 뒤 다시 Unaware로 돌아가기까지의 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Sensing", meta = (ClampMin = "0.0"))
	float LoseSightDelay = 3.0f;

	/** 시선 기준 높이 오프셋 (발밑 → 눈높이 보정, cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Sensing")
	float EyeHeightOffset = 60.0f;

private:
	/** [서버] 주기 판정 */
	void SenseTick();

	/** 특정 폰이 인식 조건(거리+각도+차폐)을 모두 만족하는가 */
	bool CanPerceive(const APawn* PlayerPawn) const;

	/** Status.Unaware 태그 부여/제거 (서버 로컬 + 리플리케이트) */
	void SetUnaware(bool bNewUnaware);

	UAbilitySystemComponent* GetOwnerASC() const;

	bool bUnaware = false;
	double LastPerceivedTime = 0.0;

	FTimerHandle SenseTimerHandle;
};
