// 프로젝트 설정의 설명 페이지에서 저작권 공지를 작성하세요.

#include "LeeGameplayAbility_Execute.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GAS_Project/MyTags.h"
#include "MotionWarpingComponent.h"

ULeeGameplayAbility_Execute::ULeeGameplayAbility_Execute(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void ULeeGameplayAbility_Execute::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
       EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
       return;
    }

    AActor* TargetActor = TriggerEventData ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
    if (TargetActor && ActorInfo->AbilitySystemComponent.IsValid())
    {
       // 1. 피해자에게 이벤트 전송
       FGameplayEventData Payload;
       Payload.Instigator = ActorInfo->AvatarActor.Get();
       Payload.Target = TargetActor;
       UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, MyTags::Events::Execution::Start, Payload);

       // 2. 무적 GE(게임플레이 이펙트) 적용
       if (InvincibleGE)
       {
          FGameplayEffectContextHandle Context = ActorInfo->AbilitySystemComponent->MakeEffectContext();
          ActiveInvincibleGEHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(InvincibleGE.GetDefaultObject(), 1.0f, Context);
       }

       // 3. 모션 워핑 타겟 설정
       // 참고: 캐릭터에 MotionWarpingComponent가 존재해야 함
       if (UMotionWarpingComponent* MotionWarpingComp = ActorInfo->AvatarActor->FindComponentByClass<UMotionWarpingComponent>())
       {
          MotionWarpingComp->AddOrUpdateWarpTargetFromTransform(TEXT("ExecutionTarget"), TargetActor->GetActorTransform());
       }

       if (ExecutionMontage)
       {
          UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ExecutionMontage);
          PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
          PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCompleted);
          PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCompleted);
          PlayMontageTask->ReadyForActivation();
       }
       else
       {
          EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
       }
    }
    else
    {
       EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
}

void ULeeGameplayAbility_Execute::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULeeGameplayAbility_Execute::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && ActiveInvincibleGEHandle.IsValid())
    {
       ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveInvincibleGEHandle);
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}