// 26.11.05 신규 - Stone 보스 AI (Behavior Tree + StateTree 혼용)

#include "BTTask_LeeActivateAbilityByTag.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/AAI/StateTree/LeeAbilityEndListener.h"

UBTTask_LeeActivateAbilityByTag::UBTTask_LeeActivateAbilityByTag()
{
	NodeName = TEXT("Lee Activate Ability By Tag");
	bNotifyTick = true;
	bNotifyTaskFinished = true;
	// 실행 중 상태(SpecHandle/리스너)를 멤버로 들고 있으므로 AI마다 인스턴스를 만든다
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_LeeActivateAbilityByTag::GetUnavailableResult() const
{
	return bSucceedWhenUnavailable ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTask_LeeActivateAbilityByTag::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ElapsedSeconds = 0.f;
	ActiveHandle = FGameplayAbilitySpecHandle();
	ActiveASC.Reset();

	const AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!ASC || !AbilityTag.IsValid())
	{
		return GetUnavailableResult();
	}

	FGameplayAbilitySpecHandle MatchedHandle;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(AbilityTag))
		{
			MatchedHandle = Spec.Handle;
			break;
		}
	}

	if (!MatchedHandle.IsValid())
	{
		UE_LOG(LogLee, Verbose, TEXT("[LeeBTT_ActivateAbility] '%s' 어빌리티가 없습니다."), *AbilityTag.ToString());
		return GetUnavailableResult();
	}

	if (!EndListener)
	{
		EndListener = NewObject<ULeeAbilityEndListener>(this);
	}
	// 활성화 전에 구독해야 즉시 끝나는 어빌리티(몽타주 미지정 등)의 종료도 놓치지 않는다
	EndListener->Bind(ASC, MatchedHandle);

	if (!ASC->TryActivateAbility(MatchedHandle))
	{
		EndListener->Unbind();
		return GetUnavailableResult();
	}

	if (EndListener->HasEnded())
	{
		EndListener->Unbind();
		return EBTNodeResult::Succeeded;
	}

	ActiveASC = ASC;
	ActiveHandle = MatchedHandle;
	return EBTNodeResult::InProgress;
}

void UBTTask_LeeActivateAbilityByTag::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	ElapsedSeconds += DeltaSeconds;

	bool bEnded = EndListener && EndListener->HasEnded();
	if (!bEnded)
	{
		UAbilitySystemComponent* ASC = ActiveASC.Get();
		const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(ActiveHandle) : nullptr;
		bEnded = (Spec == nullptr) || !Spec->IsActive();
	}

	if (bEnded)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (TimeoutSeconds > 0.f && ElapsedSeconds > TimeoutSeconds)
	{
		if (UAbilitySystemComponent* ASC = ActiveASC.Get())
		{
			ASC->CancelAbilityHandle(ActiveHandle);
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

EBTNodeResult::Type UBTTask_LeeActivateAbilityByTag::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (UAbilitySystemComponent* ASC = ActiveASC.Get())
	{
		if (ActiveHandle.IsValid())
		{
			ASC->CancelAbilityHandle(ActiveHandle);
		}
	}
	return EBTNodeResult::Aborted;
}

void UBTTask_LeeActivateAbilityByTag::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (EndListener)
	{
		EndListener->Unbind();
	}
	ActiveHandle = FGameplayAbilitySpecHandle();
	ActiveASC.Reset();

	if (TaskResult != EBTNodeResult::Aborted && !ClearBoolKeyOnFinish.IsNone())
	{
		if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
		{
			BB->SetValueAsBool(ClearBoolKeyOnFinish, false);
		}
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

FString UBTTask_LeeActivateAbilityByTag::GetStaticDescription() const
{
	return FString::Printf(TEXT("Ability: %s%s"), *AbilityTag.ToString(),
		bSucceedWhenUnavailable ? TEXT("\n(없으면 성공 처리)") : TEXT(""));
}
