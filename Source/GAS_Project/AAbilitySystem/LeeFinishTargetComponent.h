// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LeeFinishInteractionComponent.h"
#include "LeeFinishTargetComponent.generated.h"

/** Player가 다수의 Enemy 후보 중 어떤 것을 최종 타겟으로 삼을지 결정하는 규칙 */
UENUM(BlueprintType)
enum class ELeeFinishPriorityRule : uint8
{
	// 암살 > 처형, 같은 타입이면 거리 가까운 순
	AssassinationFirstThenDistance   UMETA(DisplayName = "Assassination First, Then Distance"),
	// 타입 무관 거리 가까운 순
	DistanceOnly                     UMETA(DisplayName = "Distance Only"),
	// 마지막 진입 순
	LastEntered                      UMETA(DisplayName = "Last Entered"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnLeeFinishTargetChanged, AActor*, NewTarget, ELeeFinishType, NewType);


/**
 * Player Pawn에 부착되는 처형/암살 후보 관리 컴포넌트.
 *
 * 역할:
 *  1) Enemy의 ULeeFinishInteractionComponent overlap 등록/해제 수신
 *  2) Tick에서 현재 유효한 후보를 즉석 평가하여 최종 타겟 1개 선정
 *  3) IA_Finish 입력 시 GameplayEvent 전송 (어빌리티 트리거)
 */
UCLASS(ClassGroup=(Custom),Blueprintable, meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeFinishTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeeFinishTargetComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "Lee|Finish")
	FOnLeeFinishTargetChanged OnFinishTargetChanged;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lee|Finish")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lee|Finish")
	ELeeFinishType GetCurrentType() const { return CurrentType; }

	/**
	 * 현재 타겟의 Finish 타이머 진행률(0.0~1.0)을 반환한다.
	 *
	 * - 처형(Execution) : 타겟 ASC의 Groggy GE 잔여시간 / 전체Duration 비율.
	 *                    잔여시간이 줄수록 0에 수렴 → 라디알 링이 닳아없어짐.
	 * - 암살(Assassination): 시간제한이 없으므로 항상 1.0 반환 → 풀 링 유지.
	 * - 타겟 없음 / GE 없음: 0.0 반환.
	 *
	 * 위젯 BP에서는 이 값 하나만 Material ScalarParam 'Progress'에 넘기면 됨.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lee|Finish|UI")
	float GetFinishTimerProgress() const;

	/**
	 * 현재 Finish 타입이 처형(Execution)인지 반환.
	 * 위젯 BP에서 경고색(붉은색) 전환 여부를 결정할 때 사용.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lee|Finish|UI")
	bool IsExecutionType() const { return CurrentType == ELeeFinishType::Execution; }

	// Player의 IA_Finish 입력에서 호출. 서버까지 GameplayEvent를 전달하고,
	// 어빌리티가 TriggerTag로 활성화되도록 한다.
	UFUNCTION(BlueprintCallable, Category = "Lee|Finish")
	bool TryActivateFinish();

	UFUNCTION(BlueprintCallable, Category = "Lee|Finish", meta = (DeprecatedFunction, DeprecationMessage = "Overlap boxes now call RegisterOverlap directly."))
	void RegisterEnemyComponent(ULeeFinishInteractionComponent* EnemyComp);

	UFUNCTION(BlueprintCallable, Category = "Lee|Finish", meta = (DeprecatedFunction, DeprecationMessage = "Overlap boxes now call UnregisterOverlap directly."))
	void UnregisterEnemyComponent(ULeeFinishInteractionComponent* EnemyComp);

	UFUNCTION(BlueprintCallable, Category = "Lee|Finish")
	void RegisterOverlap(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type);

	UFUNCTION(BlueprintCallable, Category = "Lee|Finish")
	void UnregisterOverlap(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type);

	void AddCandidate(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type);
	void RemoveCandidate(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type);

	bool HasCandidate(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finish|Priority")
	ELeeFinishPriorityRule PriorityRule = ELeeFinishPriorityRule::AssassinationFirstThenDistance;

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finish|Input")
	FGameplayTag ExecutionTriggerTag;

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finish|Input")
	FGameplayTag AssassinationTriggerTag;

private:
	void EvaluateCurrentTarget();
	void SetCurrentTarget(AActor* NewTarget, ELeeFinishType NewType);
	float ScoreCandidate(const ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type) const;
	ELeeFinishType ResolveFallbackTypeAfterUnregister(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType RemovedType) const;

	TMap<TWeakObjectPtr<ULeeFinishInteractionComponent>, ELeeFinishType> ActiveOverlaps;
	TMap<TWeakObjectPtr<ULeeFinishInteractionComponent>, double> EnterTimeSecondsByComp;

	TWeakObjectPtr<AActor> CurrentTarget;
	ELeeFinishType CurrentType = ELeeFinishType::None;
};
