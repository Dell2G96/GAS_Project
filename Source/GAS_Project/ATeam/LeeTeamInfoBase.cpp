// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeTeamInfoBase.h"

#include "LeeTeamSubsystem.h"
#include "Net/UnrealNetwork.h"


class FLifetimeProperty;

ALeeTeamInfoBase::ALeeTeamInfoBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TeamId(INDEX_NONE)
{
	bReplicates = true;
	bAlwaysRelevant = true;
	NetPriority = 3.0f;
	SetReplicatingMovement(false);
}

void ALeeTeamInfoBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, TeamTags);
	DOREPLIFETIME_CONDITION(ThisClass, TeamId, COND_InitialOnly);
}

void ALeeTeamInfoBase::BeginPlay()
{
	Super::BeginPlay();

	TryRegisterWithTeamSubsystem();
}

void ALeeTeamInfoBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TeamId != INDEX_NONE)
	{
		ULeeTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<ULeeTeamSubsystem>();
		if (TeamSubsystem)
		{
			// EndPlay can happen at weird times where the subsystem has already been destroyed
			TeamSubsystem->UnregisterTeamInfo(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ALeeTeamInfoBase::RegisterWithTeamSubsystem(ULeeTeamSubsystem* Subsystem)
{
	Subsystem->RegisterTeamInfo(this);
}

void ALeeTeamInfoBase::TryRegisterWithTeamSubsystem()
{
	if (TeamId != INDEX_NONE)
	{
		ULeeTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<ULeeTeamSubsystem>();
		if (ensure(TeamSubsystem))
		{
			RegisterWithTeamSubsystem(TeamSubsystem);
		}
	}
}

void ALeeTeamInfoBase::SetTeamId(int32 NewTeamId)
{
	check(HasAuthority());
	check(TeamId == INDEX_NONE);
	check(NewTeamId != INDEX_NONE);

	TeamId = NewTeamId;

	TryRegisterWithTeamSubsystem();
}

void ALeeTeamInfoBase::OnRep_TeamId()
{
	TryRegisterWithTeamSubsystem();
}



