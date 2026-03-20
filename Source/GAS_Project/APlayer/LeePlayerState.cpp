// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePlayerState.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySet.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/AAbilitySystem/AttributeSets/LeeCombatSet.h"
#include "GAS_Project/AAbilitySystem/AttributeSets/LeeHealthSet.h"
#include "GAS_Project/ACharacter/LeePawnData.h"
#include "GAS_Project/GameModes/LeeExperienceManagerComponent.h"
#include "GAS_Project/GameModes/LeeExperienceDefinition.h"
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
	// GetAuthGameMode()은 클라이언트에서 null을 반환하므로,
	// CurrentExperience 파라미터에서 직접 PawnData를 가져옴
	// Lecacy
	// if (CurrentExperience && CurrentExperience->DefaultPawnData)
	// {
	// 	SetPawnData(CurrentExperience->DefaultPawnData);
	// }

	if (ALeeGameModeBase* LeeGameMode = GetWorld()->GetAuthGameMode<ALeeGameModeBase>())
	{
		if (const ULeePawnData* NewPawnData = LeeGameMode->GetPawnDataForController(GetOwningController()))
		{
			SetPawnData(NewPawnData);
		}
		else
		{
			UE_LOG(LogLee, Error, TEXT("ALeePlayerState::OnExperienceLoaded(): Unable to find PawnData to initialize player state [%s]!"), *GetNameSafe(this));
		}
	}
}

void ALeePlayerState::SetPawnData(const class ULeePawnData* InPawnData)
{
	check(InPawnData);

	if (PawnData)
	{
		return;
	}
	PawnData = InPawnData;

	// AbilitySet은 Authority(서버)에서만 부여
	if (HasAuthority())
	{
		for (ULeeAbilitySet* AbilitySet : PawnData->AbilitySets)
		{
			if (AbilitySet)
			{
				AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
			}
		}
	}
}
