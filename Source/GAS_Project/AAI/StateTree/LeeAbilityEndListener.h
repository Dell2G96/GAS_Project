// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "LeeAbilityEndListener.generated.h"

class UAbilitySystemComponent;

/**
 * 특정 FGameplayAbilitySpecHandle 하나의 종료만 감지하는 초경량 리스너.
 * StateTree Task InstanceData(USTRUCT)는 델리게이트를 직접 보유할 수 없어
 * (인스턴스가 공유·복사되는 데이터라 델리게이트 바인딩 수명 관리가 안 된다),
 * 수명이 안정적인 UObject로 분리해 ASC::OnAbilityEnded를 구독한다.
 * 다른 어빌리티의 종료가 같은 State의 완료로 오인되지 않도록 SpecHandle을 정확히 비교한다.
 */
UCLASS()
class GAS_PROJECT_API ULeeAbilityEndListener : public UObject
{
	GENERATED_BODY()

public:
	/** 감시를 시작한다. 이미 구독 중이면 먼저 해제 후 다시 구독한다 */
	void Bind(UAbilitySystemComponent* InASC, FGameplayAbilitySpecHandle InSpecHandle);

	/** 구독을 해제한다 (ExitState/재사용 시 반드시 호출) */
	void Unbind();

	/** 감시 중인 SpecHandle이 종료되었는가 */
	bool HasEnded() const { return bEnded; }

private:
	void HandleAbilityEnded(const FAbilityEndedData& EndedData);

	TWeakObjectPtr<UAbilitySystemComponent> WatchedASC;
	FGameplayAbilitySpecHandle WatchedSpecHandle;
	FDelegateHandle DelegateHandle;
	bool bEnded = false;
};
