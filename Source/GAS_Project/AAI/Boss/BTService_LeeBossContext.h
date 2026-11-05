// 26.11.05 신규 - Stone 보스 AI (Behavior Tree + StateTree 혼용)

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_LeeBossContext.generated.h"

/** 보스 Blackboard 키 이름 (BB 자동 생성과 서비스가 같은 이름을 쓰도록 한 곳에 모음) */
namespace LeeBossBBKeys
{
	constexpr const TCHAR* TargetActor = TEXT("TargetActor");
	constexpr const TCHAR* DistanceToTarget = TEXT("DistanceToTarget");
	constexpr const TCHAR* SelfHealthRatio = TEXT("SelfHealthRatio");
	constexpr const TCHAR* Phase = TEXT("Phase");
	constexpr const TCHAR* bIsGroggy = TEXT("bIsGroggy");
	constexpr const TCHAR* bIsDead = TEXT("bIsDead");
	constexpr const TCHAR* bTargetHealing = TEXT("bTargetHealing");
	constexpr const TCHAR* bPhaseTransitionPending = TEXT("bPhaseTransitionPending");
}

/**
 * 보스 BT Root에 붙이는 서비스. 매 Interval마다 전투 상황을 Blackboard에 기록한다.
 * - 타겟: ULeeTargetSelectionComponent 결론을 그대로 사용 (StateTree Evaluator와 동일한 권위 소스)
 * - 체력 비율이 Phase2HealthThreshold 이하로 처음 내려가면 Phase=2, bPhaseTransitionPending=true (한 번만)
 * - 그로기/사망 태그 → BT 데코레이터가 즉시 하위 행동을 끊는다
 */
UCLASS(meta = (DisplayName = "Lee Boss Context"))
class GAS_PROJECT_API UBTService_LeeBossContext : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_LeeBossContext();

	/** 이 체력 비율 이하가 되면 2페이즈로 전환 */
	UPROPERTY(EditAnywhere, Category = "Lee|Boss", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Phase2HealthThreshold = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Lee|Boss|Keys")
	FName TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Lee|Boss|Keys")
	FName DistanceToTargetKey;

	UPROPERTY(EditAnywhere, Category = "Lee|Boss|Keys")
	FName SelfHealthRatioKey;

	UPROPERTY(EditAnywhere, Category = "Lee|Boss|Keys")
	FName PhaseKey;

	UPROPERTY(EditAnywhere, Category = "Lee|Boss|Keys")
	FName GroggyKey;

	UPROPERTY(EditAnywhere, Category = "Lee|Boss|Keys")
	FName DeadKey;

	UPROPERTY(EditAnywhere, Category = "Lee|Boss|Keys")
	FName TargetHealingKey;

	UPROPERTY(EditAnywhere, Category = "Lee|Boss|Keys")
	FName PhaseTransitionPendingKey;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;
};
