// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_Execution.h"

#include "CollisionDebugDrawingPublic.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GAS_Project/Interface/ExecuteInterface.h"

UGA_Execution::UGA_Execution()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Execution::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!K2_CommitAbility()) return;


	// DrawLineTraces()
	// AActor* Target;
	// IExecuteInterface* Victim = Cast<IExecuteInterface>(Target);
	// if (Victim && Victim->CanBeExecuted())
	// {
	// 	UpdateMotionWarping(Target);
	//
	// 	FGameplayEffectContextHandle ContextHandle = MakeEffectContext(Handle, ActorInfo);
	// 	ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, Cast<UGameplayEffect>(ExecutionEffect->GetDefaultObject()), 1.0f);
	//
	// 	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ExecutionMontage, 1.0f, NAME_None, false);
	// 	Task->OnBlendOut.AddDynamic(this, &ThisClass::K2_EndAbility);
	// 	Task->OnCancelled.AddDynamic(this, &ThisClass::K2_EndAbility);
	// 	Task->OnCompleted.AddDynamic(this, &ThisClass::K2_EndAbility);
	// 	Task->OnInterrupted.AddDynamic(this, &ThisClass::K2_EndAbility);
	// 	Task->ReadyForActivation();
	//
	// 	// Victim->GetExecuted(Cast<ACharacter>(GetAvatarActorFromActorInfo()), Target->PlayVictimMontage);
	// }
	// else
	// {
	// 	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	// 	return;
	// }
}

void UGA_Execution::OnExecutionFinished()
{
}

void UGA_Execution::UpdateMotionWarping(class AActor* Target)
{
}
