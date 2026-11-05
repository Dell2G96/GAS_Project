// 26.11.05 신규 - Stone 보스 AI (Behavior Tree + StateTree 혼용)

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "BTTask_LeeActivateAbilityByTag.generated.h"

class UAbilitySystemComponent;
class ULeeAbilityEndListener;

/**
 * BT용 "태그로 어빌리티 실행 후 끝날 때까지 대기" 태스크.
 * StateTree의 FLeeStateTreeTask_ActivateAbility와 같은 규칙(AssetTags 매칭 + ULeeAbilityEndListener 종료 감지)을 쓴다.
 * 보스의 페이즈 전환 포효처럼 BT가 직접 실행해야 하는 행동에 사용한다.
 *
 * bSucceedWhenUnavailable: 어빌리티가 없거나(몽타주 미지정 등) 활성화에 실패해도 성공 처리 → BT 흐름이 멈추지 않는다.
 * ClearBoolKeyOnFinish: 끝나면(성공/실패/중단 모두) 이 Bool 키를 false로 되돌린다 (예: bPhaseTransitionPending).
 */
UCLASS(meta = (DisplayName = "Lee Activate Ability By Tag"))
class GAS_PROJECT_API UBTTask_LeeActivateAbilityByTag : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_LeeActivateAbilityByTag();

	UPROPERTY(EditAnywhere, Category = "Lee|Ability")
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, Category = "Lee|Ability", meta = (ClampMin = "0.0"))
	float TimeoutSeconds = 10.f;

	UPROPERTY(EditAnywhere, Category = "Lee|Ability")
	bool bSucceedWhenUnavailable = false;

	UPROPERTY(EditAnywhere, Category = "Lee|Ability")
	FName ClearBoolKeyOnFinish;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual FString GetStaticDescription() const override;

private:
	EBTNodeResult::Type GetUnavailableResult() const;

	UPROPERTY(Transient)
	TObjectPtr<ULeeAbilityEndListener> EndListener = nullptr;

	TWeakObjectPtr<UAbilitySystemComponent> ActiveASC;
	FGameplayAbilitySpecHandle ActiveHandle;
	float ElapsedSeconds = 0.f;
};
