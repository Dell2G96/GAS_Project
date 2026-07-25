// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeCharacterMovementComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AEquipment/LeeEquipmentManagerComponent.h"
#include "GAS_Project/AEquipment/LeeMeleeWeaponInstance.h"


namespace LeeCharacter
{
	static float GroundTraceDistance = 100000.0f;
	FAutoConsoleVariableRef CVar_GroundTraceDistance(TEXT("LyraCharacter.GroundTraceDistance"), GroundTraceDistance, TEXT("Distance to trace down when generating ground information."), ECVF_Cheat);
};


ULeeCharacterMovementComponent::ULeeCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void ULeeCharacterMovementComponent::SimulateMovement(float DeltaTime)
{
	if (bHasReplicatedAcceleration)
	{
		// Preserve our replicated acceleration
		const FVector OriginalAcceleration = Acceleration;
		Super::SimulateMovement(DeltaTime);
		Acceleration = OriginalAcceleration;
	}
	else
	{
		Super::SimulateMovement(DeltaTime);
	}
}

bool ULeeCharacterMovementComponent::CanAttemptJump() const
{
	// Same as UCharacterMovementComponent's implementation but without the crouch check
	return IsJumpAllowed() &&
		(IsMovingOnGround() || IsFalling());
	// Falling included for double-jump and non-zero jump hold time, but validated by character.

}

void ULeeCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}


const FLeeCharacterGroundInfo& ULeeCharacterMovementComponent::GetGroundInfo()
{
	if (!CharacterOwner || (GFrameCounter == CachedGroundInfo.LastUpdateFrame))
	{
		return CachedGroundInfo;
	}

	if (MovementMode == MOVE_Walking)
	{
		CachedGroundInfo.GroundHitResult = CurrentFloor.HitResult;
		CachedGroundInfo.GroundDistance = 0.0f;
	}
	else
	{
		const UCapsuleComponent* CapsuleComp = CharacterOwner->GetCapsuleComponent();
		check(CapsuleComp);

		const float CapsuleHalfHeight = CapsuleComp->GetUnscaledCapsuleHalfHeight();
		const ECollisionChannel CollisionChannel = (UpdatedComponent ? UpdatedComponent->GetCollisionObjectType() : ECC_Pawn);
		const FVector TraceStart(GetActorLocation());
		const FVector TraceEnd(TraceStart.X, TraceStart.Y, (TraceStart.Z - LeeCharacter::GroundTraceDistance - CapsuleHalfHeight));

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LeeCharacterMovementComponent_GetGroundInfo), false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(QueryParams, ResponseParam);

		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, CollisionChannel, QueryParams, ResponseParam);

		CachedGroundInfo.GroundHitResult = HitResult;
		CachedGroundInfo.GroundDistance = LeeCharacter::GroundTraceDistance;

		if (MovementMode == MOVE_NavWalking)
		{
			CachedGroundInfo.GroundDistance = 0.0f;
		}
		else if (HitResult.bBlockingHit)
		{
			CachedGroundInfo.GroundDistance = FMath::Max((HitResult.Distance - CapsuleHalfHeight), 0.0f);
		}
	}

	CachedGroundInfo.LastUpdateFrame = GFrameCounter;

	return CachedGroundInfo;
}

void ULeeCharacterMovementComponent::SetReplicatedAcceleration(const FVector& InAcceleration)
{
	bHasReplicatedAcceleration = true;
	Acceleration = InAcceleration;
}

FRotator ULeeCharacterMovementComponent::GetDeltaRotation(float DeltaTime) const
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (ASC->HasMatchingGameplayTag(MyTags::Souls::Gameplay_MovementStopped))
		{
			return FRotator(0,0,0);
		}
	}

	return Super::GetDeltaRotation(DeltaTime);
}

float ULeeCharacterMovementComponent::GetMaxSpeed() const
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (ASC->HasMatchingGameplayTag(MyTags::Souls::Gameplay_MovementStopped))
		{
			return 0;
		}

		// 가드 유지 중이면 장착 무기별 배수를 최대 속도에 곱한다 (무기별 연속값).
		// Status_Guard_Active 태그는 ASC로 복제되고, 무기 배수는 정적 데이터라 서버·클라 동일 → 예측 desync 없음
		if (ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Guard_Active))
		{
			return Super::GetMaxSpeed() * GetEquippedGuardSpeedMultiplier();
		}
	}

	return Super::GetMaxSpeed();
}

// 현재 장착된 근접 무기 인스턴스의 GuardSpeedMultiplier를 반환. 장비/무기가 없으면 1.0(감속 없음)
float ULeeCharacterMovementComponent::GetEquippedGuardSpeedMultiplier() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return 1.0f;
	}

	ULeeEquipmentManagerComponent* EquipMgr = Owner->FindComponentByClass<ULeeEquipmentManagerComponent>();
	if (!EquipMgr)
	{
		return 1.0f;
	}

	ULeeMeleeWeaponInstance* Weapon = EquipMgr->GetFirstInstanceOfType<ULeeMeleeWeaponInstance>();
	if (!Weapon)
	{
		return 1.0f;
	}

	return Weapon->GetGuardSpeedMultiplier();
}

