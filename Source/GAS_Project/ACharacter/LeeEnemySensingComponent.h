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
 * 이미 인지한 플레이어(전투 중)는 완화된 조건으로 유지한다:
 *  시야각 무시, CombatLoseSightRadius까지 유지, CloseAwarenessRadius 안이면 가림도 무시.
 * 시선 가림 검사에서 다른 폰(동료 Enemy 등)의 몸은 무시한다.
 * 공격당하면(NotifyAttackedBy) 시야와 관계없이 공격자를 즉시 인지한다.
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

	UFUNCTION(BlueprintPure, Category = "Lee|Sensing")
	static ULeeEnemySensingComponent* FindEnemySensingComponent(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<ULeeEnemySensingComponent>() : nullptr;
	}

	/** [서버] 직전 SenseTick에서 CanPerceive를 통과한 플레이어 폰 목록 — AI 타겟 선정의 후보 원본 */
	UFUNCTION(BlueprintPure, Category = "Lee|Sensing")
	void GetPerceivedPawns(TArray<APawn*>& OutPawns) const;

	/** [서버] 공격당했을 때 호출 — 시야와 관계없이 공격자를 즉시 인지한 것으로 기록한다 (미인식 상태도 해제) */
	void NotifyAttackedBy(APawn* Attacker);

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

	/** [전투 중] 이미 인지한 플레이어를 계속 붙잡아 두는 거리 (cm). 이 안에서는 시야각을 보지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Sensing|Combat", meta = (ClampMin = "0.0"))
	float CombatLoseSightRadius = 1800.0f;

	/** [전투 중] 이 거리 안의 인지 중인 플레이어는 벽·지형에 가려도 놓치지 않는다 (cm, 기척으로 안다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Sensing|Combat", meta = (ClampMin = "0.0"))
	float CloseAwarenessRadius = 600.0f;

private:
	/** [서버] 주기 판정 */
	void SenseTick();

	/**
	 * 특정 폰이 인식 조건을 만족하는가. 실패 시 OutFailReason에 사유를 적는다(진단용)
	 * - 처음 발견(bAlreadyAware=false): 거리 SightRadius + 전방 시야각 + 가림 없음
	 * - 전투 중(bAlreadyAware=true): 거리 CombatLoseSightRadius, 시야각 무시, CloseAwarenessRadius 안이면 가림도 무시
	 */
	bool CanPerceive(const APawn* PlayerPawn, bool bAlreadyAware, FString* OutFailReason = nullptr) const;

	/** 눈높이 → 플레이어 중심 시선이 벽·지형에 막히지 않았는가 (다른 폰의 몸은 무시). 막히면 가린 대상을 OutBlocker에 적는다 */
	bool HasLineOfSight(const APawn* PlayerPawn, FString* OutBlocker) const;

	/** Status.Unaware 태그 부여/제거 (서버 로컬 + 리플리케이트) */
	void SetUnaware(bool bNewUnaware);

	UAbilitySystemComponent* GetOwnerASC() const;

	bool bUnaware = false;
	double LastPerceivedTime = 0.0;

	FTimerHandle SenseTimerHandle;

	/** [서버] 직전 SenseTick에서 CanPerceive를 통과한 폰들 (AAI/Token의 타겟 선정 컴포넌트가 조회) */
	TArray<TWeakObjectPtr<APawn>> PerceivedPawns;

	/** [서버] 폰별 마지막 인식 시각 — 시야를 벗어나도 LoseSightDelay 동안 후보로 유지하기 위한 기억 */
	TMap<TWeakObjectPtr<APawn>, double> LastSeenTimeByPawn;

	/** [서버·진단] 폰별 마지막 인식 실패 사유 — 기억에서 지울 때 "왜 놓쳤는지" 로그로 남기기 위함 */
	TMap<TWeakObjectPtr<APawn>, FString> LastFailReasonByPawn;
};
