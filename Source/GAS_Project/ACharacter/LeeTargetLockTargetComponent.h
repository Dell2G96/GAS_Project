// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "LeeTargetLockTargetComponent.generated.h"

/**
 * 타겟 락온 가능한 적에게 부착하는 락온 기준점 컴포넌트.
 *
 * 적 BP의 가슴/상체 높이에 배치하면, 락온 시스템이 캡슐 중심 대신 이 위치를
 * 조준·카메라 포커스·UI 인디케이터 앵커로 사용한다 — 큰 몬스터에서 캡슐 중심(배꼽 근처)을
 * 조준하게 되는 문제를 피하고, Enemy BP에서 직접 원하는 위치를 배치할 수 있게 한다.
 *
 * `ULeeTargetLockComponent::GatherCandidates()`는 이 컴포넌트가 없는 액터를 락온 후보에서
 * 제외한다 — 잠금 가능한 적은 이 컴포넌트를 명시적으로 부착해야 한다 (화이트리스트 방식).
 */
UCLASS(Blueprintable, ClassGroup = (Lee), meta = (BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeTargetLockTargetComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	ULeeTargetLockTargetComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Lee|TargetLock")
	static ULeeTargetLockTargetComponent* FindTargetLockTargetComponent(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<ULeeTargetLockTargetComponent>() : nullptr;
	}

	/** 이 대상이 지금 락온 가능한지 (예: 보스 특정 페이즈에서 일시적으로 락온 불가 처리하고 싶을 때 BP에서 false로 토글) */
	UFUNCTION(BlueprintPure, Category = "Lee|TargetLock")
	bool CanBeLocked() const { return bCanBeLocked; }

	UFUNCTION(BlueprintCallable, Category = "Lee|TargetLock")
	void SetCanBeLocked(bool bInCanBeLocked) { bCanBeLocked = bInCanBeLocked; }

	/** 락온 조준/카메라 포커스/UI 앵커로 쓸 월드 위치 (이 컴포넌트의 월드 위치) */
	UFUNCTION(BlueprintPure, Category = "Lee|TargetLock")
	FVector GetFocusLocation() const { return GetComponentLocation(); }

	/** 여러 후보 중 우선 선택 가중치 (보스 등에 양수 부여, 점수 계산에 가산) */
	UFUNCTION(BlueprintPure, Category = "Lee|TargetLock")
	float GetLockPriority() const { return Priority; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|TargetLock")
	bool bCanBeLocked = true;

	/** 후보 스코어링 시 가산되는 우선순위. 보스급 적에 높은 값(1~5)을 주면 잡몹보다 우선 선택된다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|TargetLock", meta = (ClampMin = "0.0"))
	float Priority = 0.0f;
};
