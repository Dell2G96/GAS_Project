// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "LeeThreatComponent.generated.h"

class UAbilitySystemComponent;

/** 특정 대상 1명에 대한 위협도 기록 */
USTRUCT()
struct FLeeThreatEntry
{
	GENERATED_BODY()

	UPROPERTY()
	float Threat = 0.f;

	UPROPERTY()
	double LastUpdateTime = 0.0;
};

/**
 * Enemy Pawn에 부착하는 위협도(어그로) 컴포넌트.
 * 피해를 준 Instigator(Player Pawn 기준으로 정규화)별 위협도를 누적하고,
 * 시간이 지나면 반감기(ThreatHalfLife)로 감쇠시킨다.
 * ULeeTargetSelectionComponent가 타겟 선정 1순위 기준으로 이 값을 조회한다.
 * V1은 피해로만 위협도가 오른다 (Q9 결정안 — 가드/패리/회복 등은 제외).
 * 서버 전용. 리스폰 시 초기화 정책은 ULeeTargetSelectionComponent 등 외부 호출부가
 * ResetThreat()를 호출해 처리한다 (Q7 결정안).
 */
UCLASS(Blueprintable, ClassGroup = (Lee), meta = (BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeThreatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeeThreatComponent();

	UFUNCTION(BlueprintPure, Category = "Lee|Threat")
	static ULeeThreatComponent* FindThreatComponent(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<ULeeThreatComponent>() : nullptr;
	}

	/** 위협도 반감기 (초). TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|Threat", meta = (ClampMin = "0.1"))
	float ThreatHalfLife = 15.f;

	/** 이 값 미만으로 감쇠한 엔트리는 만료된 것으로 간주해 정리 대상이 된다. TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|Threat", meta = (ClampMin = "0.0"))
	float MinThreatThreshold = 1.f;

	/** 피해량 → 위협도 환산 배율. TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|Threat", meta = (ClampMin = "0.0"))
	float DamageToThreatScale = 1.f;

	/**
	 * 위협도 기록에 감시할 체력 어트리뷰트.
	 * Enemy가 어떤 AttributeSet을 쓰는지에 따라 달라지므로 하드코딩하지 않고 에디터에서 지정한다
	 * (사망/그로기 태그를 파라미터로 노출한 것과 동일한 원칙).
	 * 기본값은 ULeeSoulsStatSet::Health. Enemy가 UCAttributeSet을 쓰면 그쪽 Health로 바꿔야 한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Threat")
	FGameplayAttribute HealthAttribute;

	/** 체력 감소를 자동으로 위협도에 반영할지 여부. false면 외부에서 AddThreat을 직접 호출해야 한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Threat")
	bool bAutoTrackHealthDamage = true;

	/** [서버] RawInstigator(Controller/PlayerState/Pawn 무엇이든)를 Player Pawn으로 정규화해 위협도를 누적한다 */
	UFUNCTION(BlueprintCallable, Category = "Lee|Threat")
	void AddThreat(AActor* RawInstigator, float Amount);

	/** [서버] 조회 시점까지 감쇠를 적용한 현재 위협도를 반환 (lazy decay) */
	UFUNCTION(BlueprintPure, Category = "Lee|Threat")
	float GetThreat(const AActor* Actor) const;

	/** [서버] 무효화된 대상 및 MinThreatThreshold 미만으로 감쇠한 엔트리를 정리한다. 매 틱 호출 금지 — 외부에서 주기적으로만 호출 */
	UFUNCTION(BlueprintCallable, Category = "Lee|Threat")
	void PruneExpiredThreats();

	/** [서버] 모든 위협도 기록을 초기화한다 (리스폰 시 등) */
	UFUNCTION(BlueprintCallable, Category = "Lee|Threat")
	void ResetThreat();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** [서버 전용] 대상별 위협도 기록 */
	TMap<TWeakObjectPtr<AActor>, FLeeThreatEntry> ThreatTable;

	/** 현재 시각 기준으로 감쇠 적용된 위협도 계산 (내부 공용 로직) */
	float ComputeDecayedThreat(const FLeeThreatEntry& Entry, double Now) const;

	/** ASC가 준비된 시점에 HealthAttribute 변경 델리게이트를 구독한다 (중복 구독 방지 포함) */
	void BindHealthAttributeDelegate();

	/** HealthAttribute 변경 델리게이트 핸들러. 감소분(=피해량)만 위협도로 환산한다 */
	void HandleHealthAttributeChanged(const FOnAttributeChangeData& Data);

	/** 바인딩 해제를 위해 보관하는 ASC와 델리게이트 핸들 */
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle HealthChangedDelegateHandle;
};
