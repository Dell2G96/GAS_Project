// Fill out your copyright notice in the Description page of Project Settings.


#include "CAniminstance.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "KismetAnimationLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/Enemy/CEnemyBase.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "GAS_Project/Characters/Player/CPlayerController.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "Kismet/KismetMathLibrary.h"


void UCAniminstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwningCharacter = Cast<ACharacter>(TryGetPawnOwner());
	if (OwningCharacter)
	{
		OwnerMovement = OwningCharacter->GetCharacterMovement();
	}
	
	// OwnerPlayerCharacter = Cast<ACPlayerCharacter>(OwnerCharacter);

}


void UCAniminstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	if (!OwningCharacter || !OwnerMovement)
	{
		return;
	}

	Speed = OwningCharacter->GetVelocity().Size2D();

	FRotator CurRotation = OwningCharacter->GetActorRotation();

	FRotator DeltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(CurRotation, PreBodyRotation);

	float TargetYawSpeed = DeltaRotator.Yaw / DeltaSeconds;

	YawSpeed = FMath::FInterpTo(YawSpeed, TargetYawSpeed, DeltaSeconds, YawSpeedSmoothLerpSpeed);

	if (FMath::Abs(TargetYawSpeed) < 0.1f)
	{
		YawSpeed = FMath::FInterpTo(YawSpeed, 0.f, DeltaSeconds, YawSpeedLerpToZeroSpeed);
	}

	SmoothedYawSpeed = FMath::Abs(YawSpeed);

	FwdSpeed = 0.0f;
	RightSpeed = 0.0f;

	bIsJumping = OwnerMovement->IsFalling();

	Direction = UKismetAnimationLibrary::CalculateDirection(OwningCharacter->GetVelocity(), CurRotation);

	PreBodyRotation = CurRotation;
	LookRotOffset = OwningCharacter->GetBaseAimRotation() - CurRotation;

	// Enemy
	if (OwningCharacter == nullptr)
	{
		const FRotator ControlRot = OwningCharacter->GetControlRotation();
		LocomotionDirection = UKismetAnimationLibrary::CalculateDirection(OwningCharacter->GetVelocity(), ControlRot);
		GroundSpeed = OwningCharacter->GetVelocity().Size2D();
		bHasAcceleration = OwnerMovement && OwnerMovement->GetCurrentAcceleration().SizeSquared2D() > 0.f;
	}
}

void UCAniminstance::OnWeaponTypeChanged(EWeaponType InPrevType, EWeaponType InNewType)
{
	WeaponType = InNewType;
}



// bool UCAniminstance::ShouldDoFullBody() const
// {
// 	return (GetSpeed() <= 0 ) && !(GetIsAimming());
// }
