// Fill out your copyright notice in the Description page of Project Settings.


#include "CPlayerAnim.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"


void UCPlayerAnim::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (OwningCharacter)
	{
		OwningPlayerCharacter = Cast<ACPlayerCharacter>(OwningCharacter);
	}
}

void UCPlayerAnim::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	
}
