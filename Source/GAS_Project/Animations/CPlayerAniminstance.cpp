// Fill out your copyright notice in the Description page of Project Settings.


#include "CPlayerAniminstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"

void UCPlayerAniminstance::NativeInitializeAnimation()
{
    OwningCharacter = Cast<ACCharacter>(TryGetPawnOwner());
    if (OwningCharacter)
    {
        OwningMovementComp = OwningCharacter->GetCharacterMovement();
    }
}

void UCPlayerAniminstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
    if(!OwningCharacter || !OwningMovementComp) return;

    GroundSpeed = OwningMovementComp->Velocity.Size2D();
	bHasAcceleration = OwningMovementComp->GetCurrentAcceleration().SizeSquared2D() > 0.f;
    bIsJumping = OwningMovementComp->IsFalling();
}
