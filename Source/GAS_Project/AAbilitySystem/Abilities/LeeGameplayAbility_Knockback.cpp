#include "LeeGameplayAbility_Knockback.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "GameFramework/Actor.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS_Project/MyTags.h"

// 생성자 — Knockback 이벤트 트리거 등록, 다른 어빌리티와 독립(Independent) 실행
ULeeGameplayAbility_Knockback::ULeeGameplayAbility_Knockback(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup    = ELeeAbilityActivationGroup::Independent;

	// 넉백 중 또 맞으면 새 넉백으로 다시 시작
	bRetriggerInstancedAbility = true;

	// 사망 중에는 밀리지 않는다
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Death_Dying);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = MyTags::Souls::Event_Combat_Knockback;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

// 이벤트의 월드 오프셋(TargetData)과 시간(EventMagnitude)으로 MoveToForce 루트모션 적용
void ULeeGameplayAbility_Knockback::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const AActor* Avatar = GetAvatarActorFromActorInfo();
	const FGameplayAbilityTargetData* OffsetData = TriggerEventData ? TriggerEventData->TargetData.Get(0) : nullptr;
	if (!Avatar || !OffsetData || TriggerEventData->EventMagnitude <= 0.0f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector WorldOffset = OffsetData->GetEndPoint();
	const FVector TargetLocation = Avatar->GetActorLocation() + WorldOffset;

	// 위로 띄우는 넉백이면 Falling으로 바꿔야 지면에 붙지 않는다 (끝나면 이전 이동 모드로 복귀 → 공중이면 자연스럽게 낙하)
	const bool bLaunchUp = WorldOffset.Z > 0.0f;

	UAbilityTask_ApplyRootMotionMoveToForce* MoveTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
		this, NAME_None, TargetLocation, TriggerEventData->EventMagnitude,
		/*bSetNewMovementMode*/bLaunchUp, /*MovementMode*/MOVE_Falling,
		/*bRestrictSpeedToExpected*/false, /*PathOffsetCurve*/nullptr,
		/*VelocityOnFinishMode*/ERootMotionFinishVelocityMode::SetVelocity, /*SetVelocityOnFinish*/FVector::ZeroVector,
		/*ClampVelocityOnFinish*/0.0f);
	MoveTask->OnTimedOut.AddDynamic(this, &ThisClass::OnKnockbackFinished);
	MoveTask->OnTimedOutAndDestinationReached.AddDynamic(this, &ThisClass::OnKnockbackFinished);
	MoveTask->ReadyForActivation();
}

// 넉백 이동 완료
void ULeeGameplayAbility_Knockback::OnKnockbackFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
