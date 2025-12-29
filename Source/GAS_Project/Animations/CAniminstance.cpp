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

	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	if (OwnerCharacter)
	{
		OwnerMovement = OwnerCharacter->GetCharacterMovement();
	}
	//UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TryGetPawnOwner());
	
	OwnerPlayerCharacter = Cast<ACPlayerCharacter>(OwnerCharacter);
	// if (DoseOwnerHaveTag(MyTags::Status::Strafing))
	// {
	// 	UE_LOG(LogTemp,Warning,TEXT("=== Strafing Is True ==="));
	// }
}

void UCAniminstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);
	//
	// if (OwnerCharacter)
	// {
	// 	FVector Velocity = OwnerCharacter->GetVelocity();
	// 	Speed =  Velocity.Length();
	// 	
	// 	FRotator BodyRotation = OwnerCharacter->GetActorRotation();
	//
	// 	// FRotator ControlRot = OwnerCharacter->GetControlRotation();
	// 	FRotator ControlRot;
	// 	if (ACPlayerController* PlayerController = Cast<ACPlayerController>(OwnerCharacter->GetController()))
	// 	{
	// 		ControlRot = OwnerCharacter->GetControlRotation();
	//
	// 	}
	// 	else if (ACEnemyBase* Enemy = Cast<ACEnemyBase>(OwnerCharacter))
	// 	{
	// 		ControlRot = Enemy->GetActorRotation();
	// 	}
	//
	// 	//수정
	// 	FRotator BodyRotationDelta = UKismetMathLibrary::NormalizedDeltaRotator(BodyRotation, PreBodyRotation);
	// 	PreBodyRotation = BodyRotation;
	//
	// 	YawSpeed = BodyRotationDelta.Yaw / DeltaTime;
	// 	float YawLerpSpeed = YawSpeedSmoothLerpSpeed;
	// 	if (YawSpeed == 0)
	// 	{
	// 		YawLerpSpeed = YawSpeedLerpToZeroSpeed;
	// 	} 
	//
	// 	
	// 	SmoothedYawSpeed = UKismetMathLibrary::FInterpTo(SmoothedYawSpeed, YawSpeed, DeltaTime, YawLerpSpeed);
	//
	// 	//수정
	// 	// FRotator ControlRot = OwnerCharacter->GetBaseAimRotation();
	// 	
	// 	LookRotOffset = UKismetMathLibrary::NormalizedDeltaRotator(ControlRot, BodyRotation);
	//
	// 	
	// 	FwdSpeed = Velocity.Dot(ControlRot.Vector());
	// 	RightSpeed = -Velocity.Dot(ControlRot.Vector().Cross(FVector::UpVector));
	//
	// 	
	// 	const FVector V = OwnerCharacter->GetVelocity();
	//
	// 	//수정
	// 	// const FRotator Base = OwnerCharacter->GetActorRotation(); // 시점 기준 원하면 GetControlRotation()
	// 	
	//
	// 	GroundSpeed = OwnerMovement->Velocity.Size2D();
	//
	// 	//수정
	// 	// Direction = UKismetAnimationLibrary::CalculateDirection(V, ControlRot);
	// 	
	// 	Direction = UKismetAnimationLibrary::CalculateDirection(V, ControlRot);
	// 	LocomotionDirection = UKismetAnimationLibrary::CalculateDirection(V, ControlRot);
	//
	// 	
	// 	
	// }
	// if (OwnerCharacter)
	// {
	// 	bIsJumping = OwnerMovement->IsFalling();
	// }

}

void UCAniminstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	if (!OwnerCharacter || !OwnerMovement)
	{
		return;
	}

	Speed = OwnerCharacter->GetVelocity().Size2D();

	FRotator CurRotation = OwnerCharacter->GetActorRotation();

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

	Direction = UKismetAnimationLibrary::CalculateDirection(OwnerCharacter->GetVelocity(), CurRotation);

	PreBodyRotation = CurRotation;
	LookRotOffset = OwnerCharacter->GetBaseAimRotation() - CurRotation;

	// Enemy
	if (OwnerPlayerCharacter == nullptr)
	{
		const FRotator ControlRot = OwnerCharacter->GetControlRotation();
		LocomotionDirection = UKismetAnimationLibrary::CalculateDirection(OwnerCharacter->GetVelocity(), ControlRot);
		GroundSpeed = OwnerCharacter->GetVelocity().Size2D();
		bHasAcceleration = OwningMovementComponent && OwningMovementComponent->GetCurrentAcceleration().SizeSquared2D() > 0.f;
	}
}

void UCAniminstance::OnWeaponTypeChanged(EWeaponType InPrevType, EWeaponType InNewType)
{
	WeaponType = InNewType;
}

bool UCAniminstance::DoseOwnerHaveTag(FGameplayTag TagToCheck) const
{
	if(!OwnerCharacter) return false;

	// 1) 정상 루트: ASC 태그가 클라에도 존재하면 그대로 True
	if (UCAbilitySystemStatics::NativeDoseActorHaveTag(OwnerCharacter, TagToCheck))
	{
		return true;
	}

	// 2) 보정 루트: AI는 Minimal Replication에서 GE로 부여된 태그가 클라에 안 보일 수 있음
	// ABP_Enemy가 체크하는 Strafing 태그에 한해서, Enemy가 복제해준 bool을 사용
	static const FGameplayTag StrafingTag = MyTags::Status::Strafing;
	if (TagToCheck.MatchesTagExact(StrafingTag))
	{
		if (const ACEnemyBase* Enemy = Cast<ACEnemyBase>(OwnerCharacter))
		{
			return Enemy->IsStrafing();
		}
	}

	return false;
	
	// return UCAbilitySystemStatics::NativeDoseActorHaveTag(OwnerCharacter,TagToCheck);
}


// bool UCAniminstance::ShouldDoFullBody() const
// {
// 	return (GetSpeed() <= 0 ) && !(GetIsAimming());
// }
