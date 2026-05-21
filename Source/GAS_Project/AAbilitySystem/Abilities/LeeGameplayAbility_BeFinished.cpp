#include "LeeGameplayAbility_BeFinished.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS_Project/AAbilitySystem/LeeGameplayAbilityTargetData_Finish.h"
#include "GAS_Project/MyTags.h"
#include "MotionWarpingComponent.h"

ULeeGameplayAbility_BeFinished::ULeeGameplayAbility_BeFinished(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup = ELeeAbilityActivationGroup::Exclusive_Blocking;

	BeFinishedEventTag = MyTags::Souls::Event_BeFinished;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = BeFinishedEventTag;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void ULeeGameplayAbility_BeFinished::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FLeeGameplayAbilityTargetData_Finish* FinishData = ExtractFinishData(TriggerEventData);
	if (!FinishData || !FinishData->VictimMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(MyTags::Souls::Status_Invincible);
	bAddedInvincibleTag = true;

	if (BeingFinishedStateGE)
	{
		const FGameplayEffectContextHandle Context = ActorInfo->AbilitySystemComponent->MakeEffectContext();
		ActiveBeingFinishedGEHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(
			BeingFinishedStateGE.GetDefaultObject(), 1.0f, Context);
	}

	if (InvincibleGE)
	{
		const FGameplayEffectContextHandle Context = ActorInfo->AbilitySystemComponent->MakeEffectContext();
		ActiveInvincibleGEHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(
			InvincibleGE.GetDefaultObject(), 1.0f, Context);
	}

	SetupVictimWarp(ActorInfo, FinishData->AttackerWorldTransform);

	UAbilityTask_PlayMontageAndWait* PlayMontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, FinishData->VictimMontage);
	PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	PlayMontageTask->ReadyForActivation();
}

const FLeeGameplayAbilityTargetData_Finish* ULeeGameplayAbility_BeFinished::ExtractFinishData(const FGameplayEventData* TriggerEventData) const
{
	if (!TriggerEventData || TriggerEventData->TargetData.Num() <= 0)
	{
		return nullptr;
	}

	const FGameplayAbilityTargetData* RawData = TriggerEventData->TargetData.Get(0);
	if (!RawData || RawData->GetScriptStruct() != FLeeGameplayAbilityTargetData_Finish::StaticStruct())
	{
		return nullptr;
	}

	return static_cast<const FLeeGameplayAbilityTargetData_Finish*>(RawData);
}

void ULeeGameplayAbility_BeFinished::SetupVictimWarp(const FGameplayAbilityActorInfo* ActorInfo, const FTransform& AttackerWorldTransform) const
{
	AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!AvatarActor)
	{
		return;
	}

	UMotionWarpingComponent* MotionWarpingComp = AvatarActor->FindComponentByClass<UMotionWarpingComponent>();
	if (!MotionWarpingComp)
	{
		return;
	}

	const FVector AttackerForward = AttackerWorldTransform.GetRotation().GetForwardVector();
	FTransform VictimWarpTransform;
	VictimWarpTransform.SetLocation(AttackerWorldTransform.GetLocation() + AttackerForward * VictimWarpDistanceFromAttacker);
	VictimWarpTransform.SetRotation((-AttackerForward).Rotation().Quaternion());

	MotionWarpingComp->AddOrUpdateWarpTargetFromTransform(VictimWarpTargetName, VictimWarpTransform);
}

void ULeeGameplayAbility_BeFinished::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULeeGameplayAbility_BeFinished::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void ULeeGameplayAbility_BeFinished::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && ActiveInvincibleGEHandle.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveInvincibleGEHandle);
		ActiveInvincibleGEHandle.Invalidate();
	}

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && ActiveBeingFinishedGEHandle.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveBeingFinishedGEHandle);
		ActiveBeingFinishedGEHandle.Invalidate();
	}

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && bAddedInvincibleTag)
	{
		ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(MyTags::Souls::Status_Invincible);
		bAddedInvincibleTag = false;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
