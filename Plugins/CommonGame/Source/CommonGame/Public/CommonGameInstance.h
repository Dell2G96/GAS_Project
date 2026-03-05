// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CommonGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class COMMONGAME_API UCommonGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	UCommonGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual int32 AddLocalPlayer(class ULocalPlayer* NewPlayer, FPlatformUserId UserId) override;
	virtual bool RemoveLocalPlayer(ULocalPlayer* ExistingPlayer) override;

	TWeakObjectPtr<class ULocalPlayer> PrimaryPlayer;
};
