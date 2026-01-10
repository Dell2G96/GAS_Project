// Fill out your copyright notice in the Description page of Project Settings.

#include "CTargetLock_Ability.h"

#include "EnhancedInputSubsystems.h"
#include "GenericTeamAgentInterface.h" // [ADDED] TeamId filtering

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"

APlayerController* UCTargetLock_Ability::GetOwningPlayerController() const
{
	// if (CurrentActorInfo && CurrentActorInfo->PlayerController.IsValid())
	// {
	// 	return CurrentActorInfo->PlayerController.Get();
	// }

	if (APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo()))
	{
		return Cast<APlayerController>(Pawn->GetController());
	}

	return nullptr;
}

ACharacter* UCTargetLock_Ability::GetOwningCharacter() const
{
	return Cast<ACharacter>(GetAvatarActorFromActorInfo());
}

bool UCTargetLock_Ability::IsOwningLocalController() const
{
	const APlayerController* PC = GetOwningPlayerController();
	return PC && PC->IsLocalController();
}

bool UCTargetLock_Ability::GetViewPoint(FVector& OutLoc, FRotator& OutRot) const
{
	OutLoc = FVector::ZeroVector;
	OutRot = FRotator::ZeroRotator;

	const APlayerController* PC = GetOwningPlayerController();
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return false;
	}

	if (PC && PC->IsLocalController() && PC->PlayerCameraManager)
	{
		OutLoc = PC->PlayerCameraManager->GetCameraLocation();
		OutRot = PC->PlayerCameraManager->GetCameraRotation();
		return true;
	}

	if (PC)
	{
		OutRot = PC->GetControlRotation();
	}
	else
	{
		OutRot = Avatar->GetActorRotation();
	}

	OutLoc = Avatar->GetActorLocation();
	if (const APawn* Pawn = Cast<APawn>(Avatar))
	{
		OutLoc = Pawn->GetPawnViewLocation();
	}

	return true;
}

bool UCTargetLock_Ability::IsActorValidForTargetLock(const AActor* InActor) const
{
	if (!InActor)
	{
		return false;
	}

	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || InActor == Avatar)
	{
		return false;
	}

	if (UCAbilitySystemStatics::NativeDoseActorHaveTag(const_cast<AActor*>(InActor), MyTags::Status::Dead))
	{
		return false;
	}

	if (!IsEnemyByTeamId(InActor))
	{
		return false;
	}

	return true;
}



void UCTargetLock_Ability::ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,	const FGameplayAbilityActivationInfo ActivationInfo,	const FGameplayEventData* TriggerEventData)
{
	const bool bLocked = TryLockOnTarget();
	if (!bLocked)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	InitTargetLockMovement();
	InitTargetMappingContext();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UCTargetLock_Ability::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ResetTargetLockMovement();
	ResetTargetLockMappingContext();
	CleanUp();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


AActor* UCTargetLock_Ability::GetInitialTargetByLineTrace() const
{
	FVector ViewLoc;
	FRotator ViewRot;
	if (!GetViewPoint(ViewLoc, ViewRot))
	{
		return nullptr;
	}

	const FVector Start = ViewLoc;
	const FVector End = Start + ViewRot.Vector() * BoxTraceDistance;

	TArray<AActor*> IgnoreActors;
	if (AActor* Avatar = const_cast<AActor*>(GetAvatarActorFromActorInfo()))
	{
		IgnoreActors.Add(Avatar);
		TArray<AActor*> Attached;
		Avatar->GetAttachedActors(Attached);
		IgnoreActors.Append(Attached);
	}

	FHitResult Hit;
	const bool bHit = UKismetSystemLibrary::LineTraceSingleForObjects(
		GetAvatarActorFromActorInfo(),
		Start,
		End,
		TraceObjectTypes,
		false,
		IgnoreActors,
		bShowPersistentDebug ? EDrawDebugTrace::Persistent : EDrawDebugTrace::None,
		Hit,
		true
	);

	if (!bHit)
	{
		return nullptr;
	}

	AActor* HitActor = Hit.GetActor();
	if (!IsActorValidForTargetLock(HitActor))
	{
		return nullptr;
	}

	return HitActor;
}

AActor* UCTargetLock_Ability::GetTargetClosestToViewCenter(const TArray<AActor*>& InActors) const
{
	if (InActors.IsEmpty())
	{
		return nullptr;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	if (!GetViewPoint(ViewLoc, ViewRot))
	{
		return nullptr;
	}

	const FVector ViewFwd = ViewRot.Vector();

	AActor* Best = nullptr;
	float BestDot = -1.f;
	float BestDistSq = FLT_MAX;

	for (AActor* A : InActors)
	{
		if (!IsActorValidForTargetLock(A))
		{
			continue;
		}

		const FVector To = (A->GetActorLocation() - ViewLoc);
		const float DistSq = To.SizeSquared();
		if (DistSq <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector Dir = To.GetSafeNormal();
		const float Dot = FVector::DotProduct(ViewFwd, Dir);
		if (Dot < MinViewCenterDot)
		{
			continue;
		}

		if (Dot > BestDot || (FMath::IsNearlyEqual(Dot, BestDot, 0.0005f) && DistSq < BestDistSq))
		{
			BestDot = Dot;
			BestDistSq = DistSq;
			Best = A;
		}
	}

	return Best;
}

AActor* UCTargetLock_Ability::GetTargetToSwitchBySideStable(const TArray<AActor*>& InActors, bool bWantRightSide) const
{
	if (!CurrentLockedActor)
	{
		return nullptr;
	}

	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return nullptr;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	if (!GetViewPoint(ViewLoc, ViewRot))
	{
		return nullptr;
	}

	const FVector AvatarLoc = Avatar->GetActorLocation();
	const FVector ToCurrent = (CurrentLockedActor->GetActorLocation() - AvatarLoc);
	if (ToCurrent.IsNearlyZero())
	{
		return nullptr;
	}

	const FVector LockForward = ToCurrent.GetSafeNormal();
	const FVector LockRight = FVector::CrossProduct(FVector::UpVector, LockForward).GetSafeNormal();

	const FVector ViewFwd = ViewRot.Vector();

	AActor* Best = nullptr;
	float BestAngleDot = -1.f;     // higher = closer angle to current
	float BestDistSq = FLT_MAX;

	for (AActor* A : InActors)
	{
		if (!IsActorValidForTargetLock(A) || A == CurrentLockedActor)
		{
			continue;
		}

		const FVector To = (A->GetActorLocation() - AvatarLoc);
		const float DistSq = To.SizeSquared();
		if (DistSq <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const FVector Dir = To.GetSafeNormal();

		const float ViewDot = FVector::DotProduct(ViewFwd, Dir);
		if (ViewDot < MinViewCenterDot)
		{
			continue;
		}

		const float Side = FVector::DotProduct(LockRight, Dir);
		if (bWantRightSide)
		{
			if (Side <= 0.02f) continue;
		}
		else
		{
			if (Side >= -0.02f) continue;
		}

		const float AngleDot = FVector::DotProduct(LockForward, Dir);

		if (AngleDot > BestAngleDot + 0.0005f ||
			(FMath::IsNearlyEqual(AngleDot, BestAngleDot, 0.0005f) && DistSq < BestDistSq))
		{
			BestAngleDot = AngleDot;
			BestDistSq = DistSq;
			Best = A;
		}
	}

	return Best;
}

void UCTargetLock_Ability::GetAvailableActorsToLock()
{
	AvailableActorsToLock.Empty();

	FVector ViewLoc;
	FRotator ViewRot;
	if (!GetViewPoint(ViewLoc, ViewRot))
	{
		return;
	}

	const FVector Start = ViewLoc;
	const FVector End = Start + ViewRot.Vector() * BoxTraceDistance;

	TArray<AActor*> IgnoreActors;
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		IgnoreActors.Add(Avatar);
		TArray<AActor*> Attached;
		Avatar->GetAttachedActors(Attached);
		IgnoreActors.Append(Attached);
	}

	TArray<FHitResult> Hits;
	UKismetSystemLibrary::BoxTraceMultiForObjects(
		GetAvatarActorFromActorInfo(),
		Start,
		End,
		TraceBoxSize / 2.f,
		ViewRot,
		TraceObjectTypes,
		false,
		IgnoreActors,
		bShowPersistentDebug ? EDrawDebugTrace::Persistent : EDrawDebugTrace::None,
		Hits,
		true
	);

	for (const FHitResult& Hit : Hits)
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			if (IsActorValidForTargetLock(HitActor))
			{
				AvailableActorsToLock.AddUnique(HitActor);
			}
		}
	}
}

bool UCTargetLock_Ability::TryLockOnTarget()
{
	GetAvailableActorsToLock();
	if (AvailableActorsToLock.IsEmpty())
	{
		return false;
	}

	CurrentLockedActor = GetInitialTargetByLineTrace();

	if (!CurrentLockedActor)
	{
		CurrentLockedActor = GetTargetClosestToViewCenter(AvailableActorsToLock);
	}

	if (!CurrentLockedActor)
	{
		return false;
	}

	//  -------------------------------------------------------------------------//
	if (ACharacter* Char = GetOwningCharacter())
	{
		if (Char->HasAuthority())
		{
			if (ACPlayerCharacter* PC = Cast<ACPlayerCharacter>(Char))
			{
				PC->bIsTargetLocked = true;
				PC->TargetLockActor = CurrentLockedActor;
			}
		}
	}
	//  -------------------------------------------------------------------------//

	if (IsOwningLocalController())
	{
		DrawTargetLockWidget();
		SetTargetLockWidgetPosition();
	}

	return true;
}


void UCTargetLock_Ability::OnTargetLockTick(float DeltaTime)
{
	if (!CurrentLockedActor ||
		UCAbilitySystemStatics::NativeDoseActorHaveTag(CurrentLockedActor, MyTags::Status::Dead) ||
		UCAbilitySystemStatics::NativeDoseActorHaveTag(GetAvatarActorFromActorInfo(), MyTags::Status::Dead))
	{
		CancelTargetLockAbility();
		return;
	}

	if (IsOwningLocalController())
	{
		SetTargetLockWidgetPosition();
	}


	const bool bShouldOverrideRotation = !UCAbilitySystemStatics::NativeDoseActorHaveTag(GetAvatarActorFromActorInfo(), MyTags::Status::Rolling);

	if (!bShouldOverrideRotation)
	{
		return;
	}

	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	const FVector From = Avatar->GetActorLocation();
	const FVector To = CurrentLockedActor->GetActorLocation();

	const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(From, To);

	if (APlayerController* PC = GetOwningPlayerController())
	{
		if (PC->IsLocalController())
		{
			const FRotator CurrentControlRot = PC->GetControlRotation();
			const FRotator TargetRot = FMath::RInterpTo(CurrentControlRot, LookAtRot, DeltaTime, TargetLockRotationInterpSpeed);
			PC->SetControlRotation(FRotator(TargetRot.Pitch, TargetRot.Yaw, 0.f));
		}
	}

	if (Avatar->HasAuthority())
	{
		const FRotator CurrentActorRot = Avatar->GetActorRotation();
		const FRotator NewActorRot = FMath::RInterpTo(CurrentActorRot, LookAtRot, DeltaTime, TargetLockRotationInterpSpeed);
		GetAvatarActorFromActorInfo()->SetActorRotation(FRotator(0.f, NewActorRot.Yaw, 0.f));
	}
}

void UCTargetLock_Ability::SwitchTarget(const FGameplayTag& InSwitchDirectionTag)
{
	if (!CurrentLockedActor)
	{
		return;
	}

	GetAvailableActorsToLock();
	if (AvailableActorsToLock.IsEmpty())
	{
		return;
	}

	AActor* NewTarget = nullptr;

	if (InSwitchDirectionTag == MyTags::Events::SwitchTarget_Left)
	{
		NewTarget = GetTargetToSwitchBySideStable(AvailableActorsToLock, false);
	}
	else if (InSwitchDirectionTag == MyTags::Events::SwitchTarget_Right)
	{
		NewTarget = GetTargetToSwitchBySideStable(AvailableActorsToLock, true);
	}

	if (NewTarget)
	{
		CurrentLockedActor = NewTarget;
		//SetTargetLockWidgetPosition();
		if (IsOwningLocalController())
		{
			SetTargetLockWidgetPosition();
		}
	}
}


void UCTargetLock_Ability::DrawTargetLockWidget()
{
	if (!IsOwningLocalController())
	{
		return;
	}

	if (TargetLockWidget || !TargetLockWidgetClass)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	TargetLockWidget = CreateWidget<UUserWidget>(PC, TargetLockWidgetClass);
	if (TargetLockWidget)
	{
		TargetLockWidget->AddToViewport();
	}
}

void UCTargetLock_Ability::SetTargetLockWidgetPosition()
{
	if (!IsOwningLocalController())
	{
		return;
	}

	if (!TargetLockWidget || !CurrentLockedActor)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	FVector2D ScreenPos;
	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, CurrentLockedActor->GetActorLocation(), ScreenPos, true))
	{
		return;
	}

	if (TargetLockWidgetSize == FVector2D::ZeroVector && TargetLockWidget->WidgetTree)
	{
		TargetLockWidget->WidgetTree->ForEachWidget([this](UWidget* W)
		{
			if (USizeBox* SB = Cast<USizeBox>(W))
			{
				TargetLockWidgetSize.X = SB->GetWidthOverride();
				TargetLockWidgetSize.Y = SB->GetHeightOverride();
			}
		});
	}

	ScreenPos -= (TargetLockWidgetSize / 2.f);
	TargetLockWidget->SetPositionInViewport(ScreenPos, false);
}


void UCTargetLock_Ability::InitTargetLockMovement()
{
	ACharacter* Char = GetOwningCharacter();
	if (!Char || !Char->GetCharacterMovement())
	{
		return;
	}

	
	UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement();

	
	if (CachedDefaultMaxWalkSpeed <= 0.f)
	{
		CachedDefaultMaxWalkSpeed = Char->GetCharacterMovement()->MaxWalkSpeed;
	}

	//  -------------------------------------------------------------------------//
	if (!bRotationFlagsCached)
	{
		bSavedUseControllerRotationYaw = Char->bUseControllerRotationYaw;
		bSavedOrientRotationToMovement = MoveComp->bOrientRotationToMovement;
		bRotationFlagsCached = true;
	}

	Char->bUseControllerRotationYaw = true;
	MoveComp->bOrientRotationToMovement = false;
	
	//  -------------------------------------------------------------------------//

	if (Char->HasAuthority() || IsOwningLocalController())
	{
		MoveComp->MaxWalkSpeed = TargetLockMaxWalkSpeed;
	}
}

void UCTargetLock_Ability::ResetTargetLockMovement()
{
	ACharacter* Char = GetOwningCharacter();
	if (!Char || !Char->GetCharacterMovement())
	{
		return;
	}
	//  -------------------------------------------------------------------------//

	UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement();

	// ✅ 회전 플래그 원복
	if (bRotationFlagsCached)
	{
		Char->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
		MoveComp->bOrientRotationToMovement = bSavedOrientRotationToMovement;

		bRotationFlagsCached = false;
	}
	//  -------------------------------------------------------------------------//

	if (CachedDefaultMaxWalkSpeed > 0.f)
	{
		if (Char->HasAuthority() || IsOwningLocalController())
		{
			Char->GetCharacterMovement()->MaxWalkSpeed = CachedDefaultMaxWalkSpeed;
		}
	}
}


void UCTargetLock_Ability::InitTargetMappingContext()
{
	if (!IsOwningLocalController())
	{
		return;
	}

	if (!TargetLockInputContext || bMappingContextApplied)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	Subsystem->AddMappingContext(TargetLockInputContext, 3);
	bMappingContextApplied = true;
}

void UCTargetLock_Ability::ResetTargetLockMappingContext()
{
	if (!IsOwningLocalController())
	{
		return;
	}

	if (!TargetLockInputContext || !bMappingContextApplied)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	Subsystem->RemoveMappingContext(TargetLockInputContext);
	bMappingContextApplied = false;
}


void UCTargetLock_Ability::CancelTargetLockAbility()
{
	CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
}

void UCTargetLock_Ability::CleanUp()
{
	AvailableActorsToLock.Empty();

	//  -------------------------------------------------------------------------//
	if (ACharacter* Char = GetOwningCharacter())
	{
		if (Char->HasAuthority())
		{
			if (ACPlayerCharacter* PC = Cast<ACPlayerCharacter>(Char))
			{
				PC->bIsTargetLocked = false;
				PC->TargetLockActor = nullptr;
			}
		}
	}
	//  -------------------------------------------------------------------------//
	CurrentLockedActor = nullptr;

	if (IsOwningLocalController() && TargetLockWidget)
	{
		TargetLockWidget->RemoveFromParent();
	}

	TargetLockWidget = nullptr;
	TargetLockWidgetSize = FVector2D::ZeroVector;
	CachedDefaultMaxWalkSpeed = 0.f;
}
