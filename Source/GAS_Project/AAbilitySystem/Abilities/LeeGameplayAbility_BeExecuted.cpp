#include "LeeGameplayAbility_BeExecuted.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS_Project/MyTags.h"

ULeeGameplayAbility_BeExecuted::ULeeGameplayAbility_BeExecuted(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
    ActivationGroup = ELeeAbilityActivationGroup::Exclusive_Blocking;
    ExecutionStartEventTag = MyTags::Souls::Event_Assassination_Start;

    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        FAbilityTriggerData TriggerData;
        TriggerData.TriggerTag = ExecutionStartEventTag;
        TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
        AbilityTriggers.Add(TriggerData);
    }
}

void ULeeGameplayAbility_BeExecuted::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

    if (InvincibleGE)
    {
        FGameplayEffectContextHandle Context = ActorInfo->AbilitySystemComponent->MakeEffectContext();
        ActiveInvincibleGEHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(InvincibleGE.GetDefaultObject(), 1.0f, Context);
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

void ULeeGameplayAbility_BeExecuted::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULeeGameplayAbility_BeExecuted::OnMontageInterrupted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void ULeeGameplayAbility_BeExecuted::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && ActiveInvincibleGEHandle.IsValid())
    {
        ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveInvincibleGEHandle);
        ActiveInvincibleGEHandle.Invalidate();
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
