// Fill out your copyright notice in the Description page of Project Settings.


#include "CTargetLock_Ability.h"

#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "GAS_Project/Characters/Player/CPlayerController.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

void UCTargetLock_Ability::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	TryLockOnTarget();
	InitTargetLockMovement();
	InitTargetMappingContext();
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UCTargetLock_Ability::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	ResetTargetLockMovement();
	ResetTargetLockMappingContext();
	CleanUp();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UCTargetLock_Ability::OnTargetLockTick(float DeltaTime)
{
	if (!CurrentLockedActor ||
		UCAbilitySystemStatics::NativeDoseActorHaveTag(CurrentLockedActor, MyTags::Status::Dead) ||
		UCAbilitySystemStatics::NativeDoseActorHaveTag(GetAvatarActorFromActorInfo(), MyTags::Status::Dead) 
		)
	{
		CancelTargetLockAbility();
		return;
	}
	SetTargetLockWidgetPosition();

	const bool bShouldOverrideRoataion = !UCAbilitySystemStatics::NativeDoseActorHaveTag(GetAvatarActorFromActorInfo(), MyTags::Status::Rolling)
		&&
		!UCAbilitySystemStatics::NativeDoseActorHaveTag(GetAvatarActorFromActorInfo(), MyTags::Status::Guarding);
	
	if (bShouldOverrideRoataion)
	{
		FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(
			GetAvatarActorFromActorInfo()->GetActorLocation(),
			CurrentLockedActor->GetActorLocation()
		);

		const FRotator CurrentControlRot = GetAvatarController()->GetControlRotation();
		const FRotator TargetRot = FMath::RInterpTo(CurrentControlRot,LookAtRot,DeltaTime,TargetLockRotationInterpSpeed);

		USpringArmComponent* CameraBoom = GetOwningAvatarCharacter()->FindComponentByClass<USpringArmComponent>();
		CameraBoom->SetWorldLocation()
		GetAvatarController()->SetControlRotation(FRotator(TargetRot.Pitch,TargetRot.Yaw,0.f));
		GetAvatarActorFromActorInfo()->SetActorRotation(FRotator(0.f,TargetRot.Yaw,0.f));
	}

}

void UCTargetLock_Ability::SwitchTarget(const FGameplayTag& InSwitchDirectionTag)
{
	GetAvailableActorsToLock();

	TArray<AActor*> ActorOnLeft;
	TArray<AActor*> ActorOnRight;
	AActor* NewTargetToLock = nullptr;

	GetAvailableActorsAroundTarget(ActorOnLeft, ActorOnRight);
	if (InSwitchDirectionTag == MyTags::Events::SwitchTarget_Left)
	{
		NewTargetToLock = GetNearestTargetFromAvailableActors(ActorOnLeft);
	}
	else
	{
		NewTargetToLock = GetNearestTargetFromAvailableActors(ActorOnRight);
	}

	if (NewTargetToLock)
	{
		CurrentLockedActor = NewTargetToLock;
	}
}

void UCTargetLock_Ability::TryLockOnTarget()
{
	GetAvailableActorsToLock();

	if (AvailableActorsToLock.IsEmpty())
	{
		CancelTargetLockAbility();
		return;
	}
	
	CurrentLockedActor = GetNearestTargetFromAvailableActors(AvailableActorsToLock);

	if (CurrentLockedActor)
	{
		DrawTargetLockWidget();
		SetTargetLockWidgetPosition();
	}
	else
	{
		CancelTargetLockAbility();
	}
}

void UCTargetLock_Ability::GetAvailableActorsToLock()
{
	AvailableActorsToLock.Empty();

	TArray<FHitResult> BoxResults;

	UKismetSystemLibrary::BoxTraceMultiForObjects(
		GetAvatarActorFromActorInfo(),
		GetAvatarActorFromActorInfo()->GetActorLocation(),
		GetAvatarActorFromActorInfo()->GetActorLocation() + GetAvatarActorFromActorInfo()->GetActorForwardVector() * BoxTraceDistance ,
		TraceBoxSize /2.f,
		GetAvatarActorFromActorInfo()->GetActorForwardVector().ToOrientationRotator(),
		BoxTraceChannel,
		false,
		TArray<AActor*>(),
		bShowPersistentDebugSphere ? EDrawDebugTrace::Persistent : EDrawDebugTrace::None,
		BoxResults,
		true);

	for (const FHitResult& TraceHit : BoxResults)
	{
		if (AActor* HitActor = TraceHit.GetActor())
		{
			if (HitActor != GetAvatarActorFromActorInfo())
			{
				AvailableActorsToLock.AddUnique(HitActor);
			}
		}
	}
}

AActor* UCTargetLock_Ability::GetNearestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors)
{
	float ClosestDistance = 0.f;
	return UGameplayStatics::FindNearestActor(GetAvatarActorFromActorInfo()->GetActorLocation(),InAvailableActors, ClosestDistance);;
}

void UCTargetLock_Ability::GetAvailableActorsAroundTarget(TArray<AActor*>& OutActorsOnLeft,
	TArray<AActor*>& OutActorsOnRight)
{
	if (!CurrentLockedActor || AvailableActorsToLock.IsEmpty())
	{
		CancelTargetLockAbility();
		return;
	}

	const FVector PlayerLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
	const FVector PlayerToCurrentNormalized = (CurrentLockedActor->GetActorLocation() - PlayerLocation).GetSafeNormal();
	for (AActor* AvailableActor : AvailableActorsToLock)
	{
		if(!AvailableActor || AvailableActor == CurrentLockedActor) continue;

		const FVector PlayerToAvailableNormalized = (AvailableActor->GetActorLocation() - PlayerLocation).GetSafeNormal();

		const FVector CrossResult = FVector::CrossProduct(PlayerToCurrentNormalized,PlayerToAvailableNormalized);

		if (CrossResult.Z>0.f)
		{
			OutActorsOnRight.AddUnique(AvailableActor);
		}
		else
		{
			OutActorsOnLeft.AddUnique(AvailableActor);
		}
	}
}

void UCTargetLock_Ability::DrawTargetLockWidget()
{
	if (!TargetLockWidget)
	{
		checkf(TargetLockWidgetClass, TEXT("Forgot to assign a valid widget class in Blueprint"));

		TargetLockWidget = CreateWidget(GetAvatarController(), TargetLockWidgetClass);
		check(TargetLockWidget);
		TargetLockWidget->AddToViewport();
	}
	
}

void UCTargetLock_Ability::SetTargetLockMovement()
{
	if (!TargetLockWidget)
	{
		checkf(TargetLockWidgetClass, TEXT("Forgot to assign a valid widget class in Blueprint"));

		TargetLockWidget = CreateWidget<UUserWidget>(GetAvatarController(), TargetLockWidgetClass);

		check(TargetLockWidget);

		TargetLockWidget->AddToViewport();
	}
}

void UCTargetLock_Ability::SetTargetLockWidgetPosition()
{
	if (!TargetLockWidget  || !CurrentLockedActor)
	{
		CancelTargetLockAbility();
		return;
	}
	
	FVector2D ScreenPosition;
	UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		GetAvatarController(),
		CurrentLockedActor->GetActorLocation(),
		ScreenPosition,
		true
	);

	if (TargetLockWidgetSize == FVector2D::ZeroVector)
	{
		TargetLockWidget->WidgetTree->ForEachWidget(
			[this](UWidget* FoundWidget)
			{
				if (USizeBox* FoundSizeBox = Cast<USizeBox>(FoundWidget))
				{
					TargetLockWidgetSize.X = FoundSizeBox->GetWidthOverride();
					TargetLockWidgetSize.Y = FoundSizeBox->GetHeightOverride();
				}
			}
		);
	}

	ScreenPosition -= (TargetLockWidgetSize / 2.f);

	TargetLockWidget->SetPositionInViewport(ScreenPosition,false);
}

void UCTargetLock_Ability::InitTargetLockMovement()
{
	CachedDefaultMaxWalkSpeed = GetOwningAvatarCharacter()->GetCharacterMovement()->MaxWalkSpeed;

	GetOwningAvatarCharacter()->GetCharacterMovement()->MaxWalkSpeed = TargetLockMaxWalkSpeed;
}

void UCTargetLock_Ability::InitTargetMappingContext()
{
	const ULocalPlayer* LocalPlayer = GetAvatarController()->GetLocalPlayer();
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	check(Subsystem)

	Subsystem->AddMappingContext(TargetLockInputContext,3);
}

void UCTargetLock_Ability::CancelTargetLockAbility()
{
	CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
}

void UCTargetLock_Ability::CleanUp()
{
	AvailableActorsToLock.Empty();
	CurrentLockedActor = nullptr;
	if (TargetLockWidget)
	{
		TargetLockWidget->RemoveFromParent();
	}

	TargetLockWidget = nullptr;

	TargetLockWidgetSize = FVector2D::ZeroVector;

	CachedDefaultMaxWalkSpeed = 0.f;
}

void UCTargetLock_Ability::ResetTargetLockMovement()
{
	if (CachedDefaultMaxWalkSpeed>0.f)
	{
		GetOwningAvatarCharacter()->GetCharacterMovement()->MaxWalkSpeed = CachedDefaultMaxWalkSpeed;
	}
}

void UCTargetLock_Ability::ResetTargetLockMappingContext()
{
	if (!GetAvatarController())
	{
		return;
	}
	
	const ULocalPlayer* LocalPlayer = GetAvatarController()->GetLocalPlayer();
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	check(Subsystem)

	Subsystem->RemoveMappingContext(TargetLockInputContext);
}
