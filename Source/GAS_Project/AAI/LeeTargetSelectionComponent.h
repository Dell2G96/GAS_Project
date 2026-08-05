// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LeeTargetSelectionComponent.generated.h"

class APawn;
class UAbilitySystemComponent;
class ULeeThreatComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FLeeOnTargetChanged, AActor* /*Old*/, AActor* /*New*/);

/**
 * Enemy의 AIController(BP)에 부착하는 타겟 선정 컴포넌트.
 * 타겟 판단의 유일한 권위 소스 — Evaluator 등 다른 코드는 GetCurrentTarget()을 읽기만 한다.
 * 원본 패턴: ULeeTargetLockComponent(함수 분해·네이밍을 그대로 미러링).
 *
 * 선정 우선순위: 1순위 위협도(ULeeThreatComponent), 2순위 최근접.
 * 후보 수집은 Enemy Pawn의 ULeeEnemySensingComponent::GetPerceivedPawns()를 그대로 사용한다.
 * 타겟이 바뀌면 이전 타겟의 ULeeAttackTokenComponent에 ReleaseAll을 호출해 토큰을 반드시 회수한다
 * (P0-5 회수 경로 4 — 가장 새기 쉬운 구멍).
 */
UCLASS(Blueprintable, ClassGroup = (Lee), meta = (BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeTargetSelectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeeTargetSelectionComponent();

	UFUNCTION(BlueprintPure, Category = "Lee|TargetSelection")
	static ULeeTargetSelectionComponent* FindTargetSelectionComponent(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<ULeeTargetSelectionComponent>() : nullptr;
	}

	/** 타겟 재선정 판정 주기 (초). TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|TargetSelection", meta = (ClampMin = "0.05"))
	float TargetUpdateInterval = 0.5f;

	/** 현재 타겟이 유지되기 위한 위협도 가중치 (배율). TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|TargetSelection", meta = (ClampMin = "1.0"))
	float CurrentTargetThreatBonus = 1.2f;

	/** 위협도 교체 판정 여유값. TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|TargetSelection", meta = (ClampMin = "0.0"))
	float ThreatSwitchMargin = 10.f;

	/** 타겟 유지 최소 시간 (초). 즉시 재선정 예외 상황에서는 무시된다. TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|TargetSelection", meta = (ClampMin = "0.0"))
	float MinTargetHoldTime = 3.0f;

	/** 최근접 판정 시 거리 교체 여유값 (cm). TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|TargetSelection", meta = (ClampMin = "0.0"))
	float DistanceSwitchMargin = 200.f;

	/**
	 * Enemy 사망 판정에 사용할 태그. 프로젝트마다 부여 주체가 달라 하드코딩하지 않고 에디터에서 지정한다 (Q3 원칙).
	 * 이 태그가 켜지면 타겟 해제 + 토큰 반납 + Souls.AI.Event.Died 발신, 꺼지면(리스폰) 위협도 초기화 후 재개.
	 * 기본값은 Lyra.Status.Death.Dead (프로젝트의 다른 Lee 컴포넌트들이 모두 이 태그로 생존을 판정한다).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|TargetSelection")
	FGameplayTag DeathTag;

	/**
	 * [신규] 그로기 판정에 사용할 태그. 이 태그가 켜지면 Souls.AI.Event.Groggy.Begin,
	 * 꺼지면 Souls.AI.Event.Groggy.End를 StateTree에 발신한다 (DeathTag와 동일한 감시 방식).
	 * 기본값은 Souls.Status.Groggy.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|TargetSelection")
	FGameplayTag GroggyTag;

	UFUNCTION(BlueprintPure, Category = "Lee|TargetSelection")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	/** [서버] 타겟은 유지한 채 현재 타겟에 걸어둔 어택 토큰만 강제 반납 (처형/StopLogic 진입 등 — 회수 경로 3) */
	UFUNCTION(BlueprintCallable, Category = "Lee|TargetSelection")
	void ForceReleaseAttackTokens();

	/** [서버] 타겟을 강제로 해제 (토큰 반납 포함). 필요 시 BP에서도 직접 호출 가능 */
	UFUNCTION(BlueprintCallable, Category = "Lee|TargetSelection")
	void ClearTarget();

	/** 타겟이 바뀔 때마다 브로드캐스트 (Old, New) */
	FLeeOnTargetChanged OnTargetChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** [서버] 판정 주기마다 호출되는 선정 로직 본체 */
	void UpdateTargetSelection();

	/** Enemy Pawn의 SensingComponent가 인지 중인 폰 중 유효 후보만 추려 반환 */
	void GatherCandidates(TArray<APawn*>& OutCandidates) const;

	/** 유효(생존) 후보인지 확인 */
	bool IsValidCandidate(const APawn* Candidate) const;

	/** 아군이 아닌지(다른 팀인지) 확인 */
	bool PassesTeamFilter(const APawn* Candidate) const;

	/** 타겟 락온/피니셔와 동일한 기준점 계산 (ULeeTargetLockTargetComponent 우선, 없으면 액터 위치) */
	FVector GetFocusLocationFor(const AActor* Actor) const;

	APawn* GetControlledEnemyPawn() const;
	ULeeThreatComponent* GetEnemyThreatComponent() const;

	/** 실제 타겟 전환 처리: 이전 타겟 토큰 반납 + 델리게이트 브로드캐스트 (이벤트 태그 발신은 호출부 책임) */
	void SetCurrentTarget(AActor* NewTarget);

	/** Target에 부착된 어택 토큰 컴포넌트에서 Requester의 모든 Claim을 반납 (모든 회수 경로의 공용 진입점) */
	void ReleaseTokensOn(AActor* Target, APawn* Requester) const;

	/** [서버] AIController 소유 StateTreeComponent에 이벤트 발신 (없으면 무시) */
	void SendAIEvent(FGameplayTag EventTag) const;

	/** Possess/UnPossess로 조종 Pawn이 바뀔 때 호출. 사망 태그 재바인딩 + 이전 Pawn의 토큰 반납 (회수 경로 5) */
	void HandlePossessedPawnChanged(APawn* NewPawn);

	/** 사망 태그 카운트 변화 처리. 1 이상이면 사망, 0이면 리스폰 (회수 경로 2) */
	void HandleDeathTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** [신규] 그로기 태그 카운트 변화 처리. 1 이상이면 그로기 진입, 0이면 해제 — StateTree 이벤트로 중계한다 */
	void HandleGroggyTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 현재 BoundPawn의 사망 태그 델리게이트 구독/해제 */
	void BindDeathTag(APawn* Pawn);
	void BindDeathTagOnBoundPawn();
	void UnbindDeathTag();

	void StartUpdateTimer();
	void StopUpdateTimer();

	TWeakObjectPtr<AActor> CurrentTarget;
	double LastSwitchTime = 0.0;
	FTimerHandle UpdateTimerHandle;

	/** 현재 사망 태그를 구독 중인 Pawn과 그 ASC (UnPossess 후에도 반납 대상을 알기 위해 보관) */
	TWeakObjectPtr<APawn> BoundPawn;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle DeathTagDelegateHandle;

	FDelegateHandle GroggyTagDelegateHandle;
	bool bIsGroggy = false;

	/** 사망 상태에서는 타겟 판정을 돌리지 않는다 */
	bool bIsDead = false;
};
