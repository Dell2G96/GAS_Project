// Fill out your copyright notice in the Description page of Project Settings.


#include "CGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "GAS_Project/Characters/Player/CPlayerController.h"
#include "GAS_Project/Components/CWeaponComponent.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"

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

class UAbilitySystemComponent* UCGameplayAbility::GetAbilitySystemComponent() const
{
	return Cast<UCAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

class ACPlayerController* UCGameplayAbility::GetHeroPlayerController() 
{
	if (!CachedPlayerController.IsValid())
	{
		CachedPlayerController = Cast<ACPlayerController>(CurrentActorInfo->PlayerController);
	}

	return CachedPlayerController.IsValid()? CachedPlayerController.Get() : nullptr;
}

class ACPlayerCharacter* UCGameplayAbility::GetHeroPlayerCharacter() 
{
	if (!CachedPlayerCharacter.IsValid())
	{
		CachedPlayerCharacter = Cast<ACPlayerCharacter>(CurrentActorInfo->AvatarActor);
	}
   
	return CachedPlayerCharacter.IsValid()? CachedPlayerCharacter.Get() : nullptr;
}

class UCWeaponComponent* UCGameplayAbility::GetPawnWeaponComp()
{
	return GetAvatarActorFromActorInfo()->FindComponentByClass<UCWeaponComponent>();

}

class UCPlayerWeaponComponent* UCGameplayAbility::GetHeroAvatarWeaponComp() 
{
	ACPlayerCharacter* Avatar = Cast<ACPlayerCharacter>(GetAvatarActorFromActorInfo());
	return Avatar->GetWeaponComponent();
}


