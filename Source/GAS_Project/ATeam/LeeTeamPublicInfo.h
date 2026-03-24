// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "LeeTeamInfoBase.h"
#include "LeeTeamPublicInfo.generated.h"

class ULeeTeamCreationComponent;
class ULeeTeamDisplayAsset;
class UObject;
struct FFrame;

UCLASS()
class GAS_PROJECT_API ALeeTeamPublicInfo : public ALeeTeamInfoBase
{
	GENERATED_BODY()

	friend ULeeTeamCreationComponent;

public:
	ALeeTeamPublicInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	ULeeTeamDisplayAsset* GetTeamDisplayAsset() const { return TeamDisplayAsset; }

private:
	UFUNCTION()
	void OnRep_TeamDisplayAsset();

	void SetTeamDisplayAsset(TObjectPtr<ULeeTeamDisplayAsset> NewDisplayAsset);

private:
	UPROPERTY(ReplicatedUsing=OnRep_TeamDisplayAsset)
	TObjectPtr<ULeeTeamDisplayAsset> TeamDisplayAsset;

};
