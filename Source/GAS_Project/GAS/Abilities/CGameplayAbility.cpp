// Fill out your copyright notice in the Description page of Project Settings.


#include "CGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"

void UCGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActivationPolicy == AbilityActivationPolicy::OnGiven)
	{
		if (ActorInfo && !Spec.IsActive())
		{
			ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
		}
	}
}

void UCGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	if (ActivationPolicy == AbilityActivationPolicy::OnGiven)
	{
		if (ActorInfo)
		{
			ActorInfo->AbilitySystemComponent->ClearAbility(Handle);
		}
	}
}

class UCWeaponComponent* UCGameplayAbility::GetWeaponComp()
{
	ACPlayerCharacter* Avatar = Cast<ACPlayerCharacter>(GetAvatarActorFromActorInfo());
	return Avatar->GetWeaponComponent();
}
