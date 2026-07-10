// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbility_TargetLock.h"

#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACharacter/LeeTargetLockComponent.h"

ULeeGameplayAbility_TargetLock::ULeeGameplayAbility_TargetLock(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 락온 토글은 데미지/판정에 관여하지 않는 순수 로컬 명령이라, 서버 왕복을 기다리지 않고
	// 즉시 반응하는 LocalPredicted가 적합하다 (ULeeGameplayAbility_Interact와 동일 정책).
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationPolicy = ELeeAbilityActivationPolicy::OnInputTriggered;
	// 상태를 갖지 않는 토글이므로 공격/구르기/피니셔와 절대 상호 차단되면 안 된다
	ActivationGroup = ELeeAbilityActivationGroup::Independent;

	AbilityTags.AddTag(MyTags::Souls::Ability_TargetLock);
}

void ULeeGameplayAbility_TargetLock::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr)
	{
		if (ULeeTargetLockComponent* Lock = ULeeTargetLockComponent::FindTargetLockComponent(Avatar))
		{
			Lock->ToggleLock();
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility*/true, /*bWasCancelled*/false);
}
