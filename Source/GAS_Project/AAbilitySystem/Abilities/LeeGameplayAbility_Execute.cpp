#include "LeeGameplayAbility_Execute.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS_Project/MyTags.h"
#include "MotionWarpingComponent.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"

ULeeGameplayAbility_Execute::ULeeGameplayAbility_Execute(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
    ActivationGroup = ELeeAbilityActivationGroup::Exclusive_Blocking;

    WarpTargetName = TEXT("ExecutionTarget");
    ExecutionEventTag = MyTags::Abilities::Execution;
    DeathEventTag = MyTags::Souls::GameplayEvent_Death;

    if (HasAnyFlags(RF_ClassDefaultObject))
    {
        FAbilityTriggerData TriggerData;
        TriggerData.TriggerTag = ExecutionEventTag;
        TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
        AbilityTriggers.Add(TriggerData);
    }
}

void ULeeGameplayAbility_Execute::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !ActorInfo->AvatarActor.IsValid())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AActor* TargetActor = TriggerEventData ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
    if (!ValidateTarget(ActorInfo, TargetActor))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    CurrentExecutionTarget = TargetActor;
    bSentDeathEvent = false;
    bAddedExecutingTag = false;

    ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(MyTags::Status::Executing);
    bAddedExecutingTag = true;

    if (ExecutingStateGE)
    {
        FGameplayEffectContextHandle Context = ActorInfo->AbilitySystemComponent->MakeEffectContext();
        ActiveExecutingGEHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(ExecutingStateGE.GetDefaultObject(), 1.0f, Context);
    }

    FGameplayEventData Payload;
    Payload.EventTag = MyTags::Souls::Event_Execution_Start;
    Payload.Instigator = ActorInfo->AvatarActor.Get();
    Payload.Target = TargetActor;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, MyTags::Souls::Event_Execution_Start, Payload);

    if (InvincibleGE)
    {
        FGameplayEffectContextHandle Context = ActorInfo->AbilitySystemComponent->MakeEffectContext();
        ActiveInvincibleGEHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(InvincibleGE.GetDefaultObject(), 1.0f, Context);
    }

    if (UMotionWarpingComponent* MotionWarpingComp = ActorInfo->AvatarActor->FindComponentByClass<UMotionWarpingComponent>())
    {
        MotionWarpingComp->AddOrUpdateWarpTargetFromTransform(WarpTargetName, TargetActor->GetActorTransform());
    }

    if (ExecutionMontage)
    {
        UAbilityTask_PlayMontageAndWait* PlayMontageTask =
            UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ExecutionMontage);
        PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
        PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
        PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
        PlayMontageTask->ReadyForActivation();
        return;
    }

    TryApplyExecutionOutcomeToTarget();
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void ULeeGameplayAbility_Execute::OnMontageCompleted()
{
    TryApplyExecutionOutcomeToTarget();
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULeeGameplayAbility_Execute::OnMontageInterrupted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

bool ULeeGameplayAbility_Execute::ValidateTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor) const
{
    if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !IsValid(TargetActor))
    {
        return false;
    }

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
    if (!TargetASC)
    {
        return false;
    }

    if (TargetASC->HasMatchingGameplayTag(MyTags::Status::Dead) ||
        TargetASC->HasMatchingGameplayTag(MyTags::Status::Executing) ||
        TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Assassinating))
    {
        return false;
    }

    const bool bIsGroggy =
        TargetASC->HasMatchingGameplayTag(MyTags::Status::Groggy) ||
        TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy);
    if (!bIsGroggy)
    {
        return false;
    }

    const AActor* AvatarActor = ActorInfo->AvatarActor.Get();
    const float Distance = FVector::Dist(AvatarActor->GetActorLocation(), TargetActor->GetActorLocation());
    return Distance <= ValidationRange;
}

void ULeeGameplayAbility_Execute::TryApplyExecutionOutcomeToTarget()
{
    if (bSentDeathEvent || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority())
    {
        return;
    }

    AActor* InstigatorActor = GetAvatarActorFromActorInfo();
    AActor* TargetActor = CurrentExecutionTarget.Get();
    if (!IsValid(InstigatorActor) || !IsValid(TargetActor))
    {
        return;
    }

    UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
    bool bAppliedDamage = false;

    if (ExecutionDamageGE && TargetASC && CurrentActorInfo->AbilitySystemComponent.IsValid())
    {
        float DamageAmount = 0.0f;
        if (const ULeeSoulsStatSet* TargetStatSet = TargetASC->GetSet<ULeeSoulsStatSet>())
        {
            DamageAmount = TargetStatSet->GetMaxHealth() * ExecutionDamageRatio;
        }

        if (DamageAmount > 0.0f)
        {
            FGameplayEffectSpecHandle SpecHandle =
                CurrentActorInfo->AbilitySystemComponent->MakeOutgoingSpec(
                    ExecutionDamageGE, 1.0f, CurrentActorInfo->AbilitySystemComponent->MakeEffectContext());

            if (SpecHandle.IsValid())
            {
                SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Damage, -DamageAmount);
                TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                bAppliedDamage = true;
            }
        }
    }

    if (TargetASC)
    {
        TargetASC->RemoveLooseGameplayTag(MyTags::Status::Groggy);
        TargetASC->RemoveLooseGameplayTag(MyTags::Status::Stun);
    }

    if (!bAppliedDamage && DeathEventTag.IsValid())
    {
        FGameplayEventData DeathPayload;
        DeathPayload.EventTag = DeathEventTag;
        DeathPayload.Instigator = InstigatorActor;
        DeathPayload.Target = TargetActor;
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, DeathEventTag, DeathPayload);
    }

    bSentDeathEvent = true;
}

void ULeeGameplayAbility_Execute::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && ActiveInvincibleGEHandle.IsValid())
    {
        ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveInvincibleGEHandle);
        ActiveInvincibleGEHandle.Invalidate();
    }

    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && ActiveExecutingGEHandle.IsValid())
    {
        ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveExecutingGEHandle);
        ActiveExecutingGEHandle.Invalidate();
    }

    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && bAddedExecutingTag)
    {
        ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(MyTags::Status::Executing);
        bAddedExecutingTag = false;
    }

    CurrentExecutionTarget.Reset();
    bSentDeathEvent = false;

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
