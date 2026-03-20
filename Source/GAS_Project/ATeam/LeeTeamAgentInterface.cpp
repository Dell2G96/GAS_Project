// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeTeamAgentInterface.h"

#include "GAS_Project/LeeLogChannels.h"


ULeeTeamAgentInterface::ULeeTeamAgentInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


void ILeeTeamAgentInterface::ConditionalBroadcastTeamChanged(TScriptInterface<ILeeTeamAgentInterface> This,
	FGenericTeamId OldTeamID, FGenericTeamId NewTeamID)
{
	if (OldTeamID != NewTeamID)
	{
		const int32 OldTeamIndex = GenericTeamIdToInteger(OldTeamID);
		const int32 NewTeamIndex = GenericTeamIdToInteger(NewTeamID);

		class UObject* ThisObj = This.GetObject();
		// UE_LOG(LogLeeTeams, Verbose, TEXT("[%s] %s assigned team %d"), *GetClientServerContextString(ThisObj), *GetPathNameSafe(ThisObj), NewTeamIndex);

		This.GetInterface()->GetTeamChangedDelegateChecked().Broadcast(ThisObj, OldTeamIndex, NewTeamIndex);
	}
}
