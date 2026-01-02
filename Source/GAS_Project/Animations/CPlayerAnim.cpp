// Fill out your copyright notice in the Description page of Project Settings.


#include "CPlayerAnim.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"


void UCPlayerAnim::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (OwningCharacter)
	{
		OwningPlayerCharacter = Cast<ACPlayerCharacter>(TryGetPawnOwner());
	}
}

void UCPlayerAnim::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	
}
