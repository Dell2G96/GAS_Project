// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "UObject/Object.h"
#include "LeeTeamAgentInterface.generated.h"

template <typename InterfaceType> class TScriptInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLeeTeamIndexChangedDelegate, class UObject*, ObjectChangingTeam, int32 , OldTeamID, int32, NewTeamID);

inline int32 GenericTeamIdToInteger(FGenericTeamId ID)
{
	return (ID == FGenericTeamId::NoTeam ) ? INDEX_NONE : (int32)ID;
	
}

inline FGenericTeamId IntegerToGenericTeamId(int32 ID)
{
	return (ID == INDEX_NONE) ? FGenericTeamId::NoTeam : FGenericTeamId((uint8)ID);
}

UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class GAS_PROJECT_API ULeeTeamAgentInterface : public UGenericTeamAgentInterface
{
	GENERATED_UINTERFACE_BODY()
};

class ILeeTeamAgentInterface : public IGenericTeamAgentInterface
{
	GENERATED_IINTERFACE_BODY()

	virtual FOnLeeTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() {return nullptr;}

	static void ConditionalBroadcastTeamChanged(TScriptInterface<ILeeTeamAgentInterface> This, FGenericTeamId OldTeamID, FGenericTeamId NewTeamID);

	FOnLeeTeamIndexChangedDelegate& GetTeamChangedDelegateChecked()
	{
		FOnLeeTeamIndexChangedDelegate* Result = GetOnTeamIndexChangedDelegate();
		check(Result);
		return *Result;
	}
	
};
