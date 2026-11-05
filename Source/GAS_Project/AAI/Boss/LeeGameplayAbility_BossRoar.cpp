#include "LeeGameplayAbility_BossRoar.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS_Project/MyTags.h"

namespace MyTags::Souls
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Boss_Stone_Roar, "Souls.Abilities.Boss.Stone.Roar");
}

ULeeGameplayAbility_BossRoar::ULeeGameplayAbility_BossRoar()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup = ELeeAbilityActivationGroup::Exclusive_Blocking;
	SetAssetTags(FGameplayTagContainer(MyTags::Souls::Boss_Stone_Roar));
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Invincible);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Death_Dying);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Death_Dead);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Groggy);
}

void ULeeGameplayAbility_BossRoar::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!RoarMontage) { EndAbility(Handle, ActorInfo, ActivationInfo, true, false); return; }
	auto* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, RoarMontage, 1.f);
	Task->OnCompleted.AddDynamic(this, &ThisClass::FinishRoar);
	Task->OnInterrupted.AddDynamic(this, &ThisClass::FinishRoar);
	Task->OnCancelled.AddDynamic(this, &ThisClass::FinishRoar);
	Task->ReadyForActivation();
}

void ULeeGameplayAbility_BossRoar::FinishRoar()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
