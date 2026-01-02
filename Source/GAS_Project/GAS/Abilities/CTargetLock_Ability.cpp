// Fill out your copyright notice in the Description page of Project Settings.


#include "CTargetLock_Ability.h"

void UCTargetLock_Ability::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UCTargetLock_Ability::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UCTargetLock_Ability::OnTargetLockTick(float DeltaTime)
{
}

void UCTargetLock_Ability::SwitchTarget(const FGameplayTag& InSwitchDirectionTag)
{
}

void UCTargetLock_Ability::TryLockOnTarget()
{
}

void UCTargetLock_Ability::GetAvailableActorsToLock()
{
}

AActor* UCTargetLock_Ability::GetNearestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors)
{
	return nullptr;
}

void UCTargetLock_Ability::GetAvailableActorsAroundTarget(TArray<AActor*>& OutActorsOnLeft,
	TArray<AActor*>& OutActorsOnRight)
{
}

void UCTargetLock_Ability::DrawTargetLockWidget()
{
}

void UCTargetLock_Ability::SetTargetLockMovement()
{
}

void UCTargetLock_Ability::InitTargetLockMovement()
{
}

void UCTargetLock_Ability::InitTargetMappingContext()
{
}

void UCTargetLock_Ability::CancelTargetLockAbility()
{
}

void UCTargetLock_Ability::CleanUp()
{
}

void UCTargetLock_Ability::ResetTargetLockMovement()
{
}

void UCTargetLock_Ability::ResetTargetLockMappingContext()
{
}
