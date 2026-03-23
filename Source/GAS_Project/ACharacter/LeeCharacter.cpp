// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeCharacter.h"

#include "LeeCharacterMovementComponent.h"
#include "LeeHealthComponent.h"
#include "LeeHeroComponent.h"
#include "LeePawnExtensionComponent.h"
#include "Components/CapsuleComponent.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/ACamera/LeeCameraComponent.h"
#include "GAS_Project/AInput/LeeInputComponent.h"
#include "GAS_Project/APlayer/LeePlayerController.h"
#include "GAS_Project/APlayer/LeePlayerState.h"
#include "Net/UnrealNetwork.h"


static FName NAME_LeeCharacterCollisionProfile_Capsule(TEXT("LeePawnCapsule"));
static FName NAME_LeeCharacterCollisionProfile_Mesh(TEXT("LeePawnMesh"));


ALeeCharacter::ALeeCharacter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<ULeeCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);
	CapsuleComp->SetCollisionProfileName(NAME_LeeCharacterCollisionProfile_Capsule);
	
	USkeletalMeshComponent* MeshComp = GetMesh();
	check(MeshComp);
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));  // Rotate mesh to be X forward since it is exported as Y forward.
	MeshComp->SetCollisionProfileName(NAME_LeeCharacterCollisionProfile_Mesh);
	
	ULeeCharacterMovementComponent* LeeMoveComp = CastChecked<ULeeCharacterMovementComponent>(GetCharacterMovement());
	LeeMoveComp->GravityScale = 1.0f;
	LeeMoveComp->MaxAcceleration = 2400.0f;
	LeeMoveComp->BrakingFrictionFactor = 1.0f;
	LeeMoveComp->BrakingFriction = 6.0f;
	LeeMoveComp->GroundFriction = 8.0f;
	LeeMoveComp->BrakingDecelerationWalking = 1400.0f;
	LeeMoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	LeeMoveComp->bAllowPhysicsRotationDuringAnimRootMotion = false;
	LeeMoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
	LeeMoveComp->bCanWalkOffLedgesWhenCrouching = true;
	LeeMoveComp->SetCrouchedHalfHeight(65.0f);
	LeeMoveComp->bUseControllerDesiredRotation = true;
    LeeMoveComp->bOrientRotationToMovement = true;
	
	PawnExtComponent = CreateDefaultSubobject<ULeePawnExtensionComponent>(TEXT("PawnExtComponent"));
	{
		PawnExtComponent->OnAbilitySystemInitialized_RegistedAndCall(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
		
		PawnExtComponent->OnAbilitySystemUnInitialized_Registed(FSimpleMulticastDelegate::FDelegate::CreateUObject(this,&ThisClass::OnAbilitySystemUnInitialized));
	}
	
	// 26.03.23 23:45 - InputComponentClass 주석 해제: Cast<ULeeInputComponent> 성공을 위해 필수
	

	// CameraComponent 생성
	{
		CameraComponent = CreateDefaultSubobject<ULeeCameraComponent>(TEXT("CameraComponent"));
		CameraComponent->SetRelativeLocation(FVector(-300.0f, 0.0f, 75.0f));
	}

	// HealthComponent 생성
	{
		HealthComponent = CreateDefaultSubobject<ULeeHealthComponent>(TEXT("HealthComponent"));
		HealthComponent->OnDeathStarted.AddDynamic(this,&ThisClass::OnDeathStarted);
		HealthComponent->OnDeathFinished.AddDynamic(this,&ThisClass::OnDeathFinished);
	}
}

void ALeeCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}


void ALeeCharacter::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

	const bool bRegisterWithSignificanceManager = !IsNetMode(NM_DedicatedServer);
	if (bRegisterWithSignificanceManager)
	{
		// if (ULeeSignificanceManager* SignificanceManager = USignificanceManager::Get<ULeeSignificanceManager>(World))
		// {
		// 	//@TODO: SignificanceManager->RegisterObject(this, (EFortSignificanceType)SignificanceType);
		// }
	}
}

void ALeeCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}


void ALeeCharacter::Reset()
{
	DisableMovementAndCollision();

	K2_OnReset();

	UninitAndDestroy();

}

void ALeeCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, ReplicatedAcceleration, COND_SimulatedOnly);
	DOREPLIFETIME(ThisClass, MyTeamID)
}

void ALeeCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		// Compress Acceleration: XY components as direction + magnitude, Z component as direct value
		const double MaxAccel = MovementComponent->MaxAcceleration;
		const FVector CurrentAccel = MovementComponent->GetCurrentAcceleration();
		double AccelXYRadians, AccelXYMagnitude;
		FMath::CartesianToPolar(CurrentAccel.X, CurrentAccel.Y, AccelXYMagnitude, AccelXYRadians);

		ReplicatedAcceleration.AccelXYRadians   = FMath::FloorToInt((AccelXYRadians / TWO_PI) * 255.0);     // [0, 2PI] -> [0, 255]
		ReplicatedAcceleration.AccelXYMagnitude = FMath::FloorToInt((AccelXYMagnitude / MaxAccel) * 255.0);	// [0, MaxAccel] -> [0, 255]
		ReplicatedAcceleration.AccelZ           = FMath::FloorToInt((CurrentAccel.Z / MaxAccel) * 127.0);   // [-MaxAccel, MaxAccel] -> [-127, 127]
	}

}

void ALeeCharacter::NotifyControllerChanged()
{
	const FGenericTeamId OldTeamId = GetGenericTeamId();

	Super::NotifyControllerChanged();

	// Update our team ID based on the controller
	if (HasAuthority() && (GetController() != nullptr))
	{
		if (ILeeTeamAgentInterface* ControllerWithTeam = Cast<ILeeTeamAgentInterface>(GetController()))
		{
			MyTeamID = ControllerWithTeam->GetGenericTeamId();
			ConditionalBroadcastTeamChanged(this, OldTeamId, MyTeamID);
		}
	}
}

ALeePlayerController* ALeeCharacter::GetLeePlayerController() const
{
	return Cast<ALeePlayerController>(GetController());
}


ALeePlayerState* ALeeCharacter::GetLeePlayerState() const
{
	return CastChecked<ALeePlayerState>(GetPlayerState(), ECastCheckedType::NullAllowed);
}



void ALeeCharacter::OnAbilitySystemInitialized()
{
	ULeeAbilitySystemComponent* LeeASC = Cast<ULeeAbilitySystemComponent>(GetAbilitySystemComponent());
	check(LeeASC);

	HealthComponent->InitializeWithAbilitySystem(LeeASC);
}

void ALeeCharacter::OnAbilitySystemUnInitialized()	
{
	HealthComponent->UninitializeAbilitySystem();
}


void ALeeCharacter::PossessedBy(AController* NewController)
{
	const FGenericTeamId OldTeamID = MyTeamID;

	Super::PossessedBy(NewController);

	PawnExtComponent->HandleControllerChanged();

	// Grab the current team ID and listen for future changes
	if (ILeeTeamAgentInterface* ControllerAsTeamProvider = Cast<ILeeTeamAgentInterface>(NewController))
	{
		MyTeamID = ControllerAsTeamProvider->GetGenericTeamId();
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnControllerChangedTeam);
	}
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}


void ALeeCharacter::UnPossessed()
{
	AController* const OldController = GetController();

	// Stop listening for changes from the old controller
	const FGenericTeamId OldTeamID = MyTeamID;
	if (ILeeTeamAgentInterface* ControllerAsTeamProvider = Cast<ILeeTeamAgentInterface>(OldController))
	{
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().RemoveAll(this);
	}

	Super::UnPossessed();

	PawnExtComponent->HandleControllerChanged();

	// Determine what the new team ID should be afterwards
	MyTeamID = DetermineNewTeamAfterPossessionEnds(OldTeamID);
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void ALeeCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	PawnExtComponent->HandleControllerChanged();
}

void ALeeCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	PawnExtComponent->HandlePlayerStateReplicated();
}

void ALeeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PawnExtComponent->SetupPlayerInputComponent();

	// 26.03.23 23:45 - InitState가 이미 DataInitialized에 도달했지만
	// Pawn->InputComponent가 null이어서 InitializePlayerInput이 호출되지 못한 경우의 보완 처리
	if (ULeeHeroComponent* HeroComp = ULeeHeroComponent::FindHeroComponent(this))
	{
		if (!HeroComp->GetReadyToBindInputs())
		{
			HeroComp->InitializePlayerInput(PlayerInputComponent);
		}
	}
}



void ALeeCharacter::InitializeGameplayTags()
{
	// Clear tags that may be lingering on the ability system from the previous pawn.
	if (ULeeAbilitySystemComponent* LeeASC = GetLeeAbilitySystemComponent())
	{
		for (const TPair<uint8, FGameplayTag>& TagMapping : MyTags::MovementModeTagMap)
		{
			if (TagMapping.Value.IsValid())
			{
				LeeASC->SetLooseGameplayTagCount(TagMapping.Value, 0);
			}
		}

		for (const TPair<uint8, FGameplayTag>& TagMapping : MyTags::CustomMovementModeTagMap
			)
		{
			if (TagMapping.Value.IsValid())
			{
				LeeASC->SetLooseGameplayTagCount(TagMapping.Value, 0);
			}
		}

		ULeeCharacterMovementComponent* LeeMoveComp = CastChecked<ULeeCharacterMovementComponent>(GetCharacterMovement());
		SetMovementModeTag(LeeMoveComp->MovementMode, LeeMoveComp->CustomMovementMode, true);
	}
}

void ALeeCharacter::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (const ULeeAbilitySystemComponent* LeeASC = GetLeeAbilitySystemComponent())
	{
		LeeASC->GetOwnedGameplayTags(TagContainer);
	}
}

bool ALeeCharacter::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	if (const ULeeAbilitySystemComponent* LeeASC = GetLeeAbilitySystemComponent())
	{
		return LeeASC->HasMatchingGameplayTag(TagToCheck);
	}

	return false;
}


bool ALeeCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const ULeeAbilitySystemComponent* LeeASC = GetLeeAbilitySystemComponent())
	{
		return LeeASC->HasAllMatchingGameplayTags(TagContainer);
	}

	return false;
}

bool ALeeCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const ULeeAbilitySystemComponent* LeeASC = GetLeeAbilitySystemComponent())
	{
		return LeeASC->HasAnyMatchingGameplayTags(TagContainer);
	}

	return false;
}

void ALeeCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	HealthComponent->DamageSelfDestruct(/*bFellOutOfWorld=*/ true);
}


class ULeeAbilitySystemComponent* ALeeCharacter::GetLeeAbilitySystemComponent() const
{
	return Cast<ULeeAbilitySystemComponent>(GetAbilitySystemComponent());
}


UAbilitySystemComponent* ALeeCharacter::GetAbilitySystemComponent() const
{
	return PawnExtComponent->GetLeeAbilitySystemComponent();
}

void ALeeCharacter::OnDeathStarted(AActor* OwningActor)
{
	DisableMovementAndCollision();
}

void ALeeCharacter::OnDeathFinished(AActor* OwningActor)
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::DestroyDueToDeath);
}

void ALeeCharacter::DisableMovementAndCollision()
{
	if (GetController())
	{
		GetController()->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	ULeeCharacterMovementComponent* LeeMoveComp = CastChecked<ULeeCharacterMovementComponent>(GetCharacterMovement());
	LeeMoveComp->StopMovementImmediately();
	LeeMoveComp->DisableMovement();
}

void ALeeCharacter::DestroyDueToDeath()
{
	K2_OnDeathFinished();

	UninitAndDestroy();
}


void ALeeCharacter::UninitAndDestroy()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		DetachFromControllerPendingDestroy();
		SetLifeSpan(0.1f);
	}

	// Uninitialize the ASC if we're still the avatar actor (otherwise another pawn already did it when they became the avatar actor)
	if (ULeeAbilitySystemComponent* LeeASC = GetLeeAbilitySystemComponent())
	{
		if (LeeASC->GetAvatarActor() == this)
		{
			PawnExtComponent->UnInitializeAbilitySystem();
		}
	}

	SetActorHiddenInGame(true);
}


void ALeeCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	ULeeCharacterMovementComponent* LeeMoveComp = CastChecked<ULeeCharacterMovementComponent>(GetCharacterMovement());

	SetMovementModeTag(PrevMovementMode, PreviousCustomMode, false);
	SetMovementModeTag(LeeMoveComp->MovementMode, LeeMoveComp->CustomMovementMode, true);
}

void ALeeCharacter::SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode, bool bTagEnabled)
{
	if (ULeeAbilitySystemComponent* LeeASC = GetLeeAbilitySystemComponent())
	{
		const FGameplayTag* MovementModeTag = nullptr;
		if (MovementMode == MOVE_Custom)
		{
			MovementModeTag = MyTags::CustomMovementModeTagMap.Find(CustomMovementMode);
		}
		else
		{
			MovementModeTag = MyTags::MovementModeTagMap.Find(MovementMode);
		}

		if (MovementModeTag && MovementModeTag->IsValid())
		{
			LeeASC->SetLooseGameplayTagCount(*MovementModeTag, (bTagEnabled ? 1 : 0));
		}
	}
}

void ALeeCharacter::ToggleCrouch()
{
	const ULeeCharacterMovementComponent* LeeMoveComp = CastChecked<ULeeCharacterMovementComponent>(GetCharacterMovement());

	if (IsCrouched() || LeeMoveComp->bWantsToCrouch)
	{
		UnCrouch();
	}
	else if (LeeMoveComp->IsMovingOnGround())
	{
		Crouch();
	}
}

void ALeeCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (ULeeAbilitySystemComponent* LeeASC = GetLeeAbilitySystemComponent())
	{
		LeeASC->SetLooseGameplayTagCount(MyTags::Lyra::Status_Crouching, 1);
	}


	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void ALeeCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (ULeeAbilitySystemComponent* LeeASC = GetLeeAbilitySystemComponent())
	{
		LeeASC->SetLooseGameplayTagCount(MyTags::Lyra::Status_Crouching, 0);
	}

	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

bool ALeeCharacter::CanJumpInternal_Implementation() const
{
	// same as ACharacter's implementation but without the crouch check
	return JumpIsAllowedInternal();
}

void ALeeCharacter::OnRep_ReplicatedAcceleration()
{
	if (ULeeCharacterMovementComponent* LeeMovementComponent = Cast<ULeeCharacterMovementComponent>(GetCharacterMovement()))
	{
		// Decompress Acceleration
		const double MaxAccel         = LeeMovementComponent->MaxAcceleration;
		const double AccelXYMagnitude = double(ReplicatedAcceleration.AccelXYMagnitude) * MaxAccel / 255.0; // [0, 255] -> [0, MaxAccel]
		const double AccelXYRadians   = double(ReplicatedAcceleration.AccelXYRadians) * TWO_PI / 255.0;     // [0, 255] -> [0, 2PI]

		FVector UnpackedAcceleration(FVector::ZeroVector);
		FMath::PolarToCartesian(AccelXYMagnitude, AccelXYRadians, UnpackedAcceleration.X, UnpackedAcceleration.Y);
		UnpackedAcceleration.Z = double(ReplicatedAcceleration.AccelZ) * MaxAccel / 127.0; // [-127, 127] -> [-MaxAccel, MaxAccel]

		LeeMovementComponent->SetReplicatedAcceleration(UnpackedAcceleration);
	}
}

void ALeeCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (GetController() == nullptr)
	{
		if (HasAuthority())
		{
			const FGenericTeamId OldTeamID = MyTeamID;
			MyTeamID = NewTeamID;
			ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
		}
		else
		{
			UE_LOG(LogLee, Error, TEXT("You can't set the team ID on a character (%s) except on the authority"), *GetPathNameSafe(this));
		}
	}
	else
	{
		UE_LOG(LogLee, Error, TEXT("You can't set the team ID on a possessed character (%s); it's driven by the associated controller"), *GetPathNameSafe(this));
	}
}

FGenericTeamId ALeeCharacter::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnLeeTeamIndexChangedDelegate* ALeeCharacter::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void ALeeCharacter::OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	const FGenericTeamId MyOldTeamID = MyTeamID;
	MyTeamID = IntegerToGenericTeamId(NewTeam);
	ConditionalBroadcastTeamChanged(this, MyOldTeamID, MyTeamID);
}

void ALeeCharacter::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

bool ALeeCharacter::UpdateSharedReplication()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		FSharedRepMovement SharedMovement;
		if (SharedMovement.FillForCharacter(this))
		{
			// Only call FastSharedReplication if data has changed since the last frame.
			// Skipping this call will cause replication to reuse the same bunch that we previously
			// produced, but not send it to clients that already received. (But a new client who has not received
			// it, will get it this frame)
			if (!SharedMovement.Equals(LastSharedReplication, this))
			{
				LastSharedReplication = SharedMovement;
				SetReplicatedMovementMode(SharedMovement.RepMovementMode);

				FastSharedReplication(SharedMovement);
			}
			return true;
		}
	}

	// We cannot fastrep right now. Don't send anything.
	return false;
}

void ALeeCharacter::FastSharedReplication_Implementation(const FSharedRepMovement& SharedRepMovement)
{
	if (GetWorld()->IsPlayingReplay())
	{
		return;
	}

	// Timestamp is checked to reject old moves.
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		// Timestamp
		SetReplicatedServerLastTransformUpdateTimeStamp(SharedRepMovement.RepTimeStamp);

		// Movement mode
		if (GetReplicatedMovementMode() != SharedRepMovement.RepMovementMode)
		{
			SetReplicatedMovementMode(SharedRepMovement.RepMovementMode);
			GetCharacterMovement()->bNetworkMovementModeChanged = true;
			GetCharacterMovement()->bNetworkUpdateReceived = true;
		}

		// Location, Rotation, Velocity, etc.
		FRepMovement& MutableRepMovement = GetReplicatedMovement_Mutable();
		MutableRepMovement = SharedRepMovement.RepMovement;

		// This also sets LastRepMovement
		OnRep_ReplicatedMovement();

		// Jump force
		SetProxyIsJumpForceApplied(SharedRepMovement.bProxyIsJumpForceApplied);

		// Crouch
		if (IsCrouched() != SharedRepMovement.bIsCrouched)
		{
			SetIsCrouched(SharedRepMovement.bIsCrouched);
			OnRep_IsCrouched();
		}
	}
}

FSharedRepMovement::FSharedRepMovement()
{
	RepMovement.LocationQuantizationLevel = EVectorQuantization::RoundTwoDecimals;
}

bool FSharedRepMovement::FillForCharacter(ACharacter* Character)
{
	if (USceneComponent* PawnRootComponent = Character->GetRootComponent())
	{
		UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement();

		RepMovement.Location = FRepMovement::RebaseOntoZeroOrigin(PawnRootComponent->GetComponentLocation(), Character);
		RepMovement.Rotation = PawnRootComponent->GetComponentRotation();
		RepMovement.LinearVelocity = CharacterMovement->Velocity;
		RepMovementMode = CharacterMovement->PackNetworkMovementMode();
		bProxyIsJumpForceApplied = Character->GetProxyIsJumpForceApplied() || (Character->JumpForceTimeRemaining > 0.0f);
		bIsCrouched = Character->IsCrouched();

		// Timestamp is sent as zero if unused
		if ((CharacterMovement->NetworkSmoothingMode == ENetworkSmoothingMode::Linear) || CharacterMovement->bNetworkAlwaysReplicateTransformUpdateTimestamp)
		{
			RepTimeStamp = CharacterMovement->GetServerLastTransformUpdateTimeStamp();
		}
		else
		{
			RepTimeStamp = 0.f;
		}

		return true;
	}
	return false;
}

bool FSharedRepMovement::Equals(const FSharedRepMovement& Other, ACharacter* Character) const
{
	if (RepMovement.Location != Other.RepMovement.Location)
	{
		return false;
	}

	if (RepMovement.Rotation != Other.RepMovement.Rotation)
	{
		return false;
	}

	if (RepMovement.LinearVelocity != Other.RepMovement.LinearVelocity)
	{
		return false;
	}

	if (RepMovementMode != Other.RepMovementMode)
	{
		return false;
	}

	if (bProxyIsJumpForceApplied != Other.bProxyIsJumpForceApplied)
	{
		return false;
	}

	if (bIsCrouched != Other.bIsCrouched)
	{
		return false;
	}

	return true;
}

bool FSharedRepMovement::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	RepMovement.NetSerialize(Ar, Map, bOutSuccess);
	Ar << RepMovementMode;
	Ar << bProxyIsJumpForceApplied;
	Ar << bIsCrouched;

	// Timestamp, if non-zero.
	uint8 bHasTimeStamp = (RepTimeStamp != 0.f);
	Ar.SerializeBits(&bHasTimeStamp, 1);
	if (bHasTimeStamp)
	{
		Ar << RepTimeStamp;
	}
	else
	{
		RepTimeStamp = 0.f;
	}

	return true;
}

