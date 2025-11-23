// Fill out your copyright notice in the Description page of Project Settings.


#include "CAniminstance.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "KismetAnimationLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "Kismet/KismetMathLibrary.h"


void UCAniminstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	if (OwnerCharacter)
	{
		OwnerMovement = OwnerCharacter->GetCharacterMovement();
		OwnerPlayerCharacter = Cast<ACPlayerCharacter>(OwnerCharacter);

	}
	UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TryGetPawnOwner());

}

void UCAniminstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (OwnerCharacter)
	{
		FVector Velocity = OwnerCharacter->GetVelocity();
		Speed =  Velocity.Length();
		FRotator BodyRotation = OwnerCharacter->GetActorRotation();
		FRotator BodyRotationDelta = UKismetMathLibrary::NormalizedDeltaRotator(BodyRotation, PreBodyRotation);
		PreBodyRotation = BodyRotation;

		YawSpeed = BodyRotationDelta.Yaw / DeltaTime;
		float YawLerpSpeed = YawSpeedSmoothLerpSpeed;
		if (YawSpeed == 0)
		{
			YawLerpSpeed = YawSpeedLerpToZeroSpeed;
		} 

		
		SmoothedYawSpeed = UKismetMathLibrary::FInterpTo(SmoothedYawSpeed, YawSpeed, DeltaTime, YawLerpSpeed);
		FRotator ControlRot = OwnerCharacter->GetBaseAimRotation();
		LookRotOffset = UKismetMathLibrary::NormalizedDeltaRotator(ControlRot, BodyRotation);

		FwdSpeed = Velocity.Dot(ControlRot.Vector());
		RightSpeed = -Velocity.Dot(ControlRot.Vector().Cross(FVector::UpVector));

		
		const FVector V = OwnerCharacter->GetVelocity();
		const FRotator Base = OwnerCharacter->GetActorRotation(); // 시점 기준 원하면 GetControlRotation()
		Direction = UKismetAnimationLibrary::CalculateDirection(V, Base);
		
		
	}
	if (OwnerCharacter)
	{
		bIsJumping = OwnerMovement->IsFalling();
	}}

void UCAniminstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
}

void UCAniminstance::OnWeaponTypeChanged(EWeaponType InPrevType, EWeaponType InNewType)
{
	WeaponType = InNewType;

}


// bool UCAniminstance::ShouldDoFullBody() const
// {
// 	return (GetSpeed() <= 0 ) && !(GetIsAimming());
// }
