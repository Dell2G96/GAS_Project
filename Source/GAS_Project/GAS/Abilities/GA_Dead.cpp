// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_Dead.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Dead::UGA_Dead()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

}

void UGA_Dead::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!K2_CommitAbility())
	{
		K2_EndAbility(); 
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character || !DeathMontage)
	{
		K2_EndAbility();
		return;
	}

	// ✅ 여기서 AbilityTask_PlayMontageAndWait 사용 → 서버/클라 모두 싱크 맞춰서 재생됨
	UAbilityTask_PlayMontageAndWait* Task =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			DeathMontage,
			1.f,
			NAME_None,
			false, 1.f, 0.f, false);

	Task->OnCompleted.AddDynamic(this, &UGA_Dead::K2_EndAbility);    // 단순히 끝나면 어빌리티 종료
	Task->OnInterrupted.AddDynamic(this, &UGA_Dead::K2_EndAbility);
	Task->OnCancelled.AddDynamic(this, &UGA_Dead::K2_EndAbility);
	Task->ReadyForActivation();

	
	
}
