// 26.11.08 신규 - Enemy 공용 반응 가드 StateTree 노드 (보스·일반 적·방패 적 공용)

#include "LeeStateTreeNodes_Guard.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"

namespace
{
	// 스태미나가 남아 있는지 (가드는 스태미나 0이면 시작할 수 없다)
	bool HasStamina(const UAbilitySystemComponent* ASC)
	{
		return ASC && ASC->GetNumericAttribute(ULeeSoulsStatSet::GetStaminaAttribute()) > 0.f;
	}

	// Ability_Guard 태그를 가진 어빌리티 스펙 핸들을 찾는다 (없으면 무효 핸들)
	FGameplayAbilitySpecHandle FindGuardSpecHandle(const UAbilitySystemComponent* ASC)
	{
		if (ASC)
		{
			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(MyTags::Souls::Ability_Guard))
				{
					return Spec.Handle;
				}
			}
		}
		return FGameplayAbilitySpecHandle();
	}
}

// 감지 결과 + 정면 + 공격 중·그로기 아님 + 스태미나 > 0
bool FLeeStateTreeCondition_CanReactWithGuard::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (!Data.bShouldGuardNow || !Data.bIsTargetInFront)
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor);
	return ASC
		&& !ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Attack_Attacking)
		&& !ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy)
		&& HasStamina(ASC);
}

// 가드를 올리고 제자리에 멈춘다
EStateTreeRunStatus FLeeStateTreeTask_ReactiveGuard::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const EStateTreeRunStatus Result = FLeeStateTreeTask_MaintainGuard::EnterState(Context, Transition);
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (Result == EStateTreeRunStatus::Running && Data.AIController)
	{
		Data.AIController->StopMovement();
	}
	return Result;
}

// 공격이 끝났거나, 가드가 풀렸거나, 퍼펙트 가드에 성공(반격 윈도우)하면 끝낸다
EStateTreeRunStatus FLeeStateTreeTask_ReactiveGuard::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor);
	if (!Data.bGuardThreatActive || !ASC
		|| !ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Guard_Active)
		|| ASC->HasMatchingGameplayTag(MyTags::Souls::Status_CounterWindow))
	{
		return EStateTreeRunStatus::Succeeded;
	}
	return FLeeStateTreeTask_MaintainGuard::Tick(Context, DeltaTime);
}

// 완료 판정에서 빠진다 — 같은 State의 이동 Task가 끝나야 State가 끝난다
FLeeStateTreeTask_ShieldGuard::FLeeStateTreeTask_ShieldGuard()
{
	bConsideredForCompletion = false;
}

// 가드를 올린다. 스태미나 부족 등으로 실패해도 Tick에서 다시 시도하므로 Running을 유지한다
EStateTreeRunStatus FLeeStateTreeTask_ShieldGuard::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FLeeStateTreeTask_MaintainGuard::EnterState(Context, Transition);
	return EStateTreeRunStatus::Running;
}

// 가드를 계속 유지하고, 타격 직전 신호가 오면 퍼펙트 판정 시간을 다시 연다
EStateTreeRunStatus FLeeStateTreeTask_ShieldGuard::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Data.Actor);
	if (!ASC)
	{
		return EStateTreeRunStatus::Running;
	}

	if (Data.AIController && Data.TargetActor)
	{
		Data.AIController->SetFocus(Data.TargetActor);
	}

	// 가드가 풀려 있으면(가드 브레이크 뒤 회복 등) 다시 든다
	if (!ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Guard_Active)
		&& !ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy)
		&& HasStamina(ASC))
	{
		const FGameplayAbilitySpecHandle Handle = FindGuardSpecHandle(ASC);
		if (Handle.IsValid() && ASC->TryActivateAbility(Handle))
		{
			Data.ActivatedSpecHandle = Handle;
		}
		return EStateTreeRunStatus::Running;
	}

	if (Data.bShouldRefreshPerfect && ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Guard_Active))
	{
		FGameplayEventData Payload;
		Payload.EventTag = MyTags::Souls::Event_Guard_RefreshPerfectWindow;
		Payload.Instigator = Data.TargetActor;
		Payload.Target = Data.Actor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Data.Actor, MyTags::Souls::Event_Guard_RefreshPerfectWindow, Payload);
	}
	return EStateTreeRunStatus::Running;
}
