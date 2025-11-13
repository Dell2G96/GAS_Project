// Fill out your copyright notice in the Description page of Project Settings.


#include "CPlayerState.h"

#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/CAttributeSet.h"

ACPlayerState::ACPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UCAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UCAttributeSet>("AttributeSet");
}

UAbilitySystemComponent* ACPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;

}
