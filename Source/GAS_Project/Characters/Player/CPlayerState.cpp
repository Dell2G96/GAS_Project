// Fill out your copyright notice in the Description page of Project Settings.


#include "CPlayerState.h"

#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/CAttributeSet.h"

ACPlayerState::ACPlayerState()
{
	// 커스텀 Ability System Component 생성
	AbilitySystemComponent = CreateDefaultSubobject<UCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
    
	// Attribute Set 생성
	AttributeSet = CreateDefaultSubobject<UCAttributeSet>(TEXT("AttributeSet"));
}
