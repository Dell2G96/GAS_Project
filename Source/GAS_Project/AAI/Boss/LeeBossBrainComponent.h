// 26.11.05 신규 - Stone 보스 AI (Behavior Tree + StateTree 혼용)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "LeeBossBrainComponent.generated.h"

class UBehaviorTree;
class APawn;
class ALeeBossEncounterManager;
enum class ELeeBossEncounterPhase : uint8;

/**
 * 보스 AIController(BP)에 붙여, 기존 StateTreeAIComponent 대신 Behavior Tree를 실행하게 하는 컴포넌트.
 *
 * - 컨트롤러 클래스를 바꾸지 않는다(기존 Enemy 컨트롤러 BP 계층/퍼셉션/팀 설정을 그대로 재사용).
 * - Possess 직후(BP의 On Possess가 StateTree StartLogic을 호출한 뒤) StateTree를 멈추고 BossBehaviorTree를 돌린다.
 * - BT는 페이즈/그로기/사망 같은 "큰 흐름"만 담당하고, 실제 전투 패턴은 BT 안의 Run State Tree 노드가 StateTree로 처리한다.
 */
UCLASS(Blueprintable, ClassGroup = (Lee), meta = (BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeBossBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeeBossBrainComponent();

	/** 보스가 실행할 Behavior Tree (Blackboard 포함) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|Boss")
	TObjectPtr<UBehaviorTree> BossBehaviorTree;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lee|Boss", meta=(ClampMin="0", ClampMax="1"))
	float Phase2HealthThreshold = 0.5f;

	// 26.11.08 이동 - 반응 가드 확률은 StateTree 평가기(Lee Combat Context)의 GuardChance로 옮겼다

	/** [서버] StateTree를 멈추고 BossBehaviorTree를 (재)시작한다 */
	UFUNCTION(BlueprintCallable, Category = "Lee|Boss")
	void StartBossBrain();

	/** Experience가 현재 Pawn의 컨트롤러에 주입한 보스 컴포넌트를 찾는다. */
	static ULeeBossBrainComponent* FindForPawn(const AActor* Actor);

	/** 공격 비용으로 고갈되면 방금 사용한 비용만큼 회복할 때까지 쉬게 한다. */
	void BeginAttackStaminaRecovery(float SpentStamina);
	bool IsRecoveringAttackStamina();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FTimerHandle StartTimer;
	bool bStarted = false;
	float AttackStaminaRecoveryTarget = 0.f;
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** 기존 StateTree와 보스 BT의 동시 실행 방지 */
	void StopStateTreeLogic() const;

	// 26.11.12 추가 - 보스전 관리자가 있으면 전투 단계 전까지 AI를 멈추고 기다린다. 기다려야 하면 true
	bool WaitForEncounterCombat();

	// 26.11.12 추가 - 보스전 단계가 전투가 되면 BT를 시작한다
	void HandleEncounterPhaseChanged(ELeeBossEncounterPhase NewPhase);

	TWeakObjectPtr<ALeeBossEncounterManager> EncounterManager;
	FDelegateHandle EncounterPhaseHandle;
};
