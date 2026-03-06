// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePlayerState.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySet.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/AAbilitySystem/AttributeSets/LeeCombatSet.h"
#include "GAS_Project/AAbilitySystem/AttributeSets/LeeHealthSet.h"
#include "GAS_Project/ACharacter/LeePawnData.h"
#include "GAS_Project/GameModes/LeeExperienceManagerComponent.h"
#include "GAS_Project/GameModes/LeeGameModeBase.h"

ALeePlayerState::ALeePlayerState(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<ULeeAbilitySystemComponent>(this, "AbilitySystemComponent");

	CreateDefaultSubobject<ULeeHealthSet>(TEXT("HealthSet"));
	CreateDefaultSubobject<ULeeCombatSet>(TEXT("CombatSet"));
}

void ALeePlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent)
	{
		FGameplayAbilityActorInfo* ActorInfo = AbilitySystemComponent->AbilityActorInfo.Get();
		check(ActorInfo->OwnerActor == this);
		check(ActorInfo->OwnerActor == ActorInfo->AvatarActor);
	}
	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());

	const AGameStateBase* GameState = GetWorld()->GetGameState();
	check(GameState);

	ULeeExperienceManagerComponent* ExperienceManagerComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
	check(ExperienceManagerComponent);

	ExperienceManagerComponent->CallOrRegister_OnExperienceLoaded(FOnLeeExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void ALeePlayerState::OnExperienceLoaded(const class ULeeExperienceDefinition* CurrentExperience)
{
	if (ALeeGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ALeeGameModeBase>())
	{
		const ULeePawnData* NewPawnData = GameMode->GetPawnDataForController(GetOwningController());
		check(NewPawnData);

		SetPawnData(NewPawnData);
	}
}

void ALeePlayerState::SetPawnData(const class ULeePawnData* InPawnData)
{
	check(InPawnData);

	check(!PawnData);
	PawnData = InPawnData;

	for (ULeeAbilitySet* AbilitySet : PawnData->AbilitySets)
	{
		if (AbilitySet)
		{
			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
			
		}
	}
	
}