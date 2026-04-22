#include "LeeGameplayAbility_BeAssassinated.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS_Project/MyTags.h"

ULeeGameplayAbility_BeAssassinated::ULeeGameplayAbility_BeAssassinated(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup = ELeeAbilityActivationGroup::Exclusive_Blocking;
	AssassinationStartEventTag = MyTags::Souls::Event_Assassination_Start;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = AssassinationStartEventTag;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void ULeeGameplayAbility_BeAssassinated::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(MyTags::Souls::Status_Assassinated);
	bAddedAssassinatedTag = true;

	if (InvincibleGE)
	{
		FGameplayEffectContextHandle Context = ActorInfo->AbilitySystemComponent->MakeEffectContext();
		ActiveInvincibleGEHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(
			InvincibleGE.GetDefaultObject(), 1.0f, Context);
	}

	if (VictimMontage)
	{
		UAbilityTask_PlayMontageAndWait* PlayMontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, VictimMontage);
		PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
		PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		PlayMontageTask->ReadyForActivation();
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void ULeeGameplayAbility_BeAssassinated::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULeeGameplayAbility_BeAssassinated::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void ULeeGameplayAbility_BeAssassinated::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && ActiveInvincibleGEHandle.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveInvincibleGEHandle);
		ActiveInvincibleGEHandle.Invalidate();
	}

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && bAddedAssassinatedTag)
	{
		ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(MyTags::Souls::Status_Assassinated);
		bAddedAssassinatedTag = false;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
