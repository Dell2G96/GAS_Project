// Fill out your copyright notice in the Description page of Project Settings.


#include "CommonLocalPlayer.h"

UCommonLocalPlayer::UCommonLocalPlayer()
	:Super(FObjectInitializer::Get())
{
	
}

FDelegateHandle UCommonLocalPlayer::CallAndRegister_OnPlayerStateSet(FPlayerStateSetDelegate::FDelegate Delegate)
{
	APlayerController* PC = GetPlayerController(GetWorld());
	APlayerState* PlayerState = PC ? PC->PlayerState : nullptr;
	if (PlayerState)
	{
		Delegate.Execute(this, PlayerState);
	}
	return OnPlayerStateSet.Add(Delegate);
}
