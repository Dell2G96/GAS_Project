// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Info.h"
#include "GAS_Project/System/LeeGameplayTagStack.h"

#include "LeeTeamInfoBase.generated.h"

namespace EEndPlayReason { enum Type : int; }

class ULeeTeamCreationComponent;
class ULeeTeamSubsystem;
class UObject;
struct FFrame;

UCLASS(Abstract)
class ALeeTeamInfoBase : public AInfo
{
	GENERATED_BODY()

public:
	ALeeTeamInfoBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	int32 GetTeamId() const { return TeamId; }

	//~AActor interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

protected:
	virtual void RegisterWithTeamSubsystem(ULeeTeamSubsystem* Subsystem);
	void TryRegisterWithTeamSubsystem();

private:
	void SetTeamId(int32 NewTeamId);

	UFUNCTION()
	void OnRep_TeamId();

public:
	friend ULeeTeamCreationComponent;

	UPROPERTY(Replicated)
	FLeeGameplayTagStackContainer TeamTags;

private:
	UPROPERTY(ReplicatedUsing=OnRep_TeamId)
	int32 TeamId;
};
