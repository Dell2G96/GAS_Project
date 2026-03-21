// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePlayerState.h"

#include "LeePlayerController.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySet.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/AAbilitySystem/AttributeSets/LeeCombatSet.h"
#include "GAS_Project/ACharacter/LeePawnData.h"
#include "GAS_Project/ACharacter/LeePawnExtensionComponent.h"
#include "GAS_Project/GameModes/LeeExperienceManagerComponent.h"
#include "GAS_Project/GameModes/LeeExperienceDefinition.h"
#include "GAS_Project/GameModes/LeeGameModeBase.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"
#include "Net/UnrealNetwork.h"


const FName ALeePlayerState::NAME_LeeAbilityReady("LeeAbilitiesReady");

ALeePlayerState::ALeePlayerState(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
	,MyPlayerConnectionType(ELeePlayerConnectionType::Player)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<ULeeAbilitySystemComponent>(this, "AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	HealthSet = CreateDefaultSubobject<ULeeSoulsStatSet>(TEXT("HealthSet"));

	SetNetUpdateFrequency(100.f);

	MyTeamID = FGenericTeamId::NoTeam;
	MySquadID = INDEX_NONE;

	
	// CreateDefaultSubobject<ULeeHealthSet>(TEXT("HealthSet"));
	// CreateDefaultSubobject<ULeeCombatSet>(TEXT("CombatSet"));
}

void ALeePlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ALeePlayerState::Reset()
{
	Super::Reset();
}

void ALeePlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);
	if (ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
	{
		PawnExtComp->CheckDefaultInitialization();
	}
}

void ALeePlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
}

void ALeePlayerState::OnDeactivated()
{
	bool bDestroyDeactivatedPlayerState = false;

	switch (GetPlayerConnectionType())
	{
		case ELeePlayerConnectionType::Player:
		case ELeePlayerConnectionType::InactivePlayer:
			bDestroyDeactivatedPlayerState = true;
			break;
		default:
			bDestroyDeactivatedPlayerState = true;
			break;
	}

	SetPlayerConnectionType(ELeePlayerConnectionType::InactivePlayer);

	if (bDestroyDeactivatedPlayerState)
	{
		Destroy();
	}

}

void ALeePlayerState::OnReactivated()
{
	if (GetPlayerConnectionType() == ELeePlayerConnectionType::InactivePlayer)
	{
		SetPlayerConnectionType(ELeePlayerConnectionType::Player);
	}
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


void ALeePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyPlayerConnectionType, SharedParams)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyTeamID, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MySquadID, SharedParams);

	SharedParams.Condition = ELifetimeCondition::COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedViewRotation, SharedParams);

	DOREPLIFETIME(ThisClass, StatTags);	
}

FRotator ALeePlayerState::GetReplicatedViewRotation() const
{
	return ReplicatedViewRotation;
}

void ALeePlayerState::SetReplicatedViewRotation(const FRotator& NewRotation)
{
	if (NewRotation != ReplicatedViewRotation)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ReplicatedViewRotation, this);
		ReplicatedViewRotation = NewRotation;
	}
}

class ALeePlayerController* ALeePlayerState::GetLeePlayerController()
{
	return Cast<ALeePlayerController>(GetOwner());
}

class UAbilitySystemComponent* ALeePlayerState::GetAbilitySystemComponent() const
{
	return GetLeeAbilitySystemComponent();
}


void ALeePlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// ToDo : 수정 필요한가..?
	check(AbilitySystemComponent)
	{
		FGameplayAbilityActorInfo* ActorInfo = AbilitySystemComponent->AbilityActorInfo.Get();
		check(ActorInfo->OwnerActor == this);
		check(ActorInfo->OwnerActor == ActorInfo->AvatarActor);
	}
	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());

	UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && World->GetNetMode() != NM_Client)
	{
		const AGameStateBase* GameState = GetWorld()->GetGameState();
		check(GameState);

		ULeeExperienceManagerComponent* ExperienceManagerComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
		check(ExperienceManagerComponent);

		ExperienceManagerComponent->CallOrRegister_OnExperienceLoaded(FOnLeeExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
	}
}


void ALeePlayerState::SetPawnData(const class ULeePawnData* InPawnData)
{
	check(InPawnData);
	
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		UE_LOG(LogLee, Error, TEXT("Trying to set PawnData [%s] on player state [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(this), *GetNameSafe(PawnData));
		return;
	}
	
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);
	PawnData = InPawnData;

	for (ULeeAbilitySet* AbilitySet : PawnData->AbilitySets)
	{
		if (AbilitySet)
		{
			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
		}
		
	}
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_LeeAbilityReady);

	ForceNetUpdate();

	// lecacy
	{
		// // AbilitySet은 Authority(서버)에서만 부여
		// if (HasAuthority())
		// {
		// 	for (ULeeAbilitySet* AbilitySet : PawnData->AbilitySets)
		// 	{
		// 		if (AbilitySet)
		// 		{
		// 			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
		// 		}
		// 	}
		// }
	}

}

void ALeePlayerState::OnRep_PawnData()
{
}

void ALeePlayerState::SetPlayerConnectionType(ELeePlayerConnectionType NewType)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyPlayerConnectionType, this);
	MyPlayerConnectionType = NewType;
}

void ALeePlayerState::SetSquadId(int32 NewSquadId)
{
	if (HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MySquadID, this);
		MySquadID = NewSquadId;
	}
}


void ALeePlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (HasAuthority())
	{
		const FGenericTeamId OldTeamID = MyTeamID;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyTeamID, this);
		MyTeamID = NewTeamID;
		ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
	}
	else
	{
		UE_LOG(LogLee, Error, TEXT("Cannot set team for %s on non-authority"), *GetPathName(this));
	}

}

FGenericTeamId ALeePlayerState::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnLeeTeamIndexChangedDelegate* ALeePlayerState::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void ALeePlayerState::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void ALeePlayerState::OnRep_MySquadID()
{
}


void ALeePlayerState::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void ALeePlayerState::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 ALeePlayerState::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool ALeePlayerState::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

void ALeePlayerState::ClientBroadcastMessage_Implementation(const FLeeVerbMessage Message)
{
	// This check is needed to prevent running the action when in standalone mode
	if (GetNetMode() == NM_Client)
	{
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(Message.Verb, Message);
	}
}













