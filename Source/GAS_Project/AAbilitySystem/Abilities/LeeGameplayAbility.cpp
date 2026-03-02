// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameplayAbility.h"

ULeeGameplayAbility::ULeeGameplayAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	ActivationPolicy = ELeeAbilityActivationPolicy::OnInputTriggered;
}
