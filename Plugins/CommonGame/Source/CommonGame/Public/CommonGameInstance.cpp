// Fill out your copyright notice in the Description page of Project Settings.


#include "CommonGameInstance.h"

#include "CommonLocalPlayer.h"
#include "GameUIManagerSubsystem.h"

UCommonGameInstance::UCommonGameInstance(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	
}

int32 UCommonGameInstance::AddLocalPlayer(class ULocalPlayer* NewPlayer, FPlatformUserId UserId)
{
	int32 ReturnValue = Super::AddLocalPlayer(NewPlayer, UserId);
	if (ReturnValue != INDEX_NONE)
	{
		if (!PrimaryPlayer.IsValid())
		{
			PrimaryPlayer = NewPlayer;
		}

		// GameUIManagerSubsystem을 통해 NotifyPlayerAdded() 호출로 GameLayoyt을 추가
		GetSubsystem<UGameUIManagerSubsystem>()->NotifyPlayerAdd(Cast<UCommonLocalPlayer>(NewPlayer));

	}
	return ReturnValue;
}

bool UCommonGameInstance::RemoveLocalPlayer(ULocalPlayer* ExistingPlayer)
{
	if (PrimaryPlayer == ExistingPlayer)
	{
		PrimaryPlayer.Reset();
	}
	
	GetSubsystem<UGameUIManagerSubsystem>()->NotifyPlayerDestroyed(Cast<UCommonLocalPlayer>(ExistingPlayer));
	return Super::RemoveLocalPlayer(ExistingPlayer);
}
