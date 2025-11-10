// Fill out your copyright notice in the Description page of Project Settings.


#include "CCharacter.h"

#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GAS_Project/GAS/CAttributeSet.h"
#include "GAS_Project/GAS/Abilities/CGameplayAbility.h"


// Sets default values
ACCharacter::ACCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	GetMesh()->bReceivesDecals = false;

	AbilitySystemComponent = CreateDefaultSubobject<UCAbilitySystemComponent>("AbilitySystemComponent");
	AttributeSet = CreateDefaultSubobject<UCAttributeSet>("AttributeSet");
}

UAbilitySystemComponent* ACCharacter::GetAbilitySystemComponent() const
{
	return GetAbilitySystemComponent();
}

void ACCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		
	}
}


