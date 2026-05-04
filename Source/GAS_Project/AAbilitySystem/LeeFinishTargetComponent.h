// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LeeFinishInteractionComponent.h"
#include "LeeFinishTargetComponent.generated.h"

class UIndicatorDescriptor;
class ULeeIndicatorManagerComponent;
class UUserWidget;

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
 *  1) Enemy의 ULeeFinishInteractionComponent로부터 후보 등록/해제 이벤트 수신
 *  2) 우선순위 규칙에 따라 최종 타겟 1개 선정
 *  3) 최종 타겟에 대한 Indicator 부착/해제 (spine_03, 가슴)
 *  4) IA_Finish 입력 시 GameplayEvent 전송 (어빌리티 트리거)
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

	// Player의 IA_Finish 입력에서 호출. 서버까지 GameplayEvent를 전달하고,
	// 어빌리티가 TriggerTag로 활성화되도록 한다.
	UFUNCTION(BlueprintCallable, Category = "Lee|Finish")
	bool TryActivateFinish();

	// Enemy에 컴포넌트가 스폰/등록되었을 때 호출해 구독을 건다.
	// BeginPlay 시점에 레벨에 이미 존재하는 Enemy들은 자동 탐색/구독.
	UFUNCTION(BlueprintCallable, Category = "Lee|Finish")
	void RegisterEnemyComponent(ULeeFinishInteractionComponent* EnemyComp);

	UFUNCTION(BlueprintCallable, Category = "Lee|Finish")
	void UnregisterEnemyComponent(ULeeFinishInteractionComponent* EnemyComp);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finish|Priority")
	ELeeFinishPriorityRule PriorityRule = ELeeFinishPriorityRule::AssassinationFirstThenDistance;

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finish|Indicator")
	TSoftClassPtr<UUserWidget> FinishPromptWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finish|Indicator")
	FVector IndicatorWorldOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finish|Input")
	FGameplayTag ExecutionTriggerTag;

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Finish|Input")
	FGameplayTag AssassinationTriggerTag;

private:
	UFUNCTION()
	void OnEnemyCandidateEntered(AActor* EnemyActor, AActor* PlayerActor, ELeeFinishType Type);

	UFUNCTION()
	void OnEnemyCandidateLeft(AActor* EnemyActor, AActor* PlayerActor, ELeeFinishType Type);

	void RecomputeTarget();
	void SetCurrentTarget(AActor* NewTarget, ELeeFinishType NewType);

	void AttachIndicatorTo(AActor* Target);
	void DetachIndicator();

	bool IsOwningPlayer(AActor* PlayerActor) const;

	struct FFinishCandidate
	{
		TWeakObjectPtr<ULeeFinishInteractionComponent> SourceComp;
		TWeakObjectPtr<AActor> Enemy;
		ELeeFinishType Type = ELeeFinishType::None;
		double EnterTimeSeconds = 0.0;
	};

	TArray<FFinishCandidate> Candidates;

	// Candidates에 없더라도 구독 해제가 누락되지 않도록 등록된 컴포넌트를 별도 추적
	TSet<TWeakObjectPtr<ULeeFinishInteractionComponent>> SubscribedEnemyComponents;

	TWeakObjectPtr<AActor> CurrentTarget;
	ELeeFinishType CurrentType = ELeeFinishType::None;

	UPROPERTY(Transient)
	TObjectPtr<UIndicatorDescriptor> ActiveIndicator;
};
