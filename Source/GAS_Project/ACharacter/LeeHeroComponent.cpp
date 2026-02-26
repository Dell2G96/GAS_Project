// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeHeroComponent.h"

#include "EnhancedInputSubsystems.h"
#include "LeePawnExtensionComponent.h"
#include "GAS_Project/LeeLogChannels.h"

#include "Components/GameFrameworkComponentManager.h"
#include "LeeGameplayTags.h"

#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACamera/LeeCameraComponent.h"
#include "GAS_Project/AInput/LeeInputComponent.h"
#include "GAS_Project/APlayer/LeePlayerController.h"
#include "GAS_Project/APlayer/LeePlayerState.h"
#include "GAS_Project/GameModes/LeeExperienceManagerComponent.h"


const FName ULeeHeroComponent::NAME_ActorFeatureName("Hero");

ULeeHeroComponent::ULeeHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
}

void ULeeHeroComponent::OnRegister()
{
	Super::OnRegister();

	{
		if (!GetPawn<APawn>())
		{
			UE_LOG(LogLee, Error, TEXT("ULeeHeroComponent::OnRegister() - Pawn이 존재하지 않습니다. Pawn이 존재하는 Actor에 컴포넌트를 추가해주세요. Actor : %s"), *GetOwner()->GetName());
		}
	}

	 RegisterInitStateFeature();
}

void ULeeHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	// PawnExtensionComponent에 대해서 (PawnExtension Feature) OnActorInitStateChanged() 관찰하도록 (Observing)
	BindOnActorInitStateChanged(ULeePawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// InitState_Spawned로 초기화
	ensure(TryToChangeInitState(MyTags::InitState::Spawned));
	
	// ForceUpdate 진행
	CheckDefaultInitialization();
}

void ULeeHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void ULeeHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	const FGameplayTag& InitTag = FGameplayTag();

	if (Params.FeatureName == ULeePawnExtensionComponent::NAME_ActorFeatureName)
	{
		// - CanChangeInitState 확인
		if (Params.FeatureState == MyTags::InitState::DataInitialized)
		{
			CheckDefaultInitialization();
		}
	}
	
}

bool ULeeHeroComponent::CanChangeInitState(class UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	const FGameplayTag& InitTags = FGameplayTag();
	APawn* Pawn = GetPawn<APawn>();
	ALeePlayerState* LeePS = GetPlayerState<ALeePlayerState>();

	if (!CurrentState.IsValid() && DesiredState == MyTags::InitState::Spawned)
	{
		if (Pawn)
		{
			return true;
		}
	}

	if (CurrentState == MyTags::InitState::Spawned && DesiredState == MyTags::InitState::DataAvailable)
	{
		if (!LeePS)
		{
			return false;
		}
		return true;
	}

	if (CurrentState == MyTags::InitState::DataAvailable && DesiredState == MyTags::InitState::DataInitialized)
	{
		// PawnExtensionComponent가 DataInitialized될 때까지 기다림 (== 모든 Feature Component가 DataAvailable인 상태)
		return LeePS && Manager->HasFeatureReachedInitState(Pawn, ULeePawnExtensionComponent::NAME_ActorFeatureName, MyTags::InitState::DataInitialized);
	}

	// DataInitialized -> GameplayReady
	if (CurrentState == MyTags::InitState::DataInitialized && DesiredState == MyTags::InitState::GameplayReady)
	{
		return true;
	}

	return false;
	
}

void ULeeHeroComponent::HandleChangeInitState(class UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState)
{

	const FGameplayTag& InitTags = FGameplayTag();

	if (CurrentState == MyTags::InitState::DataAvailable && DesiredState == MyTags::InitState::DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		ALeePlayerState* LeePS = GetPlayerState<ALeePlayerState>();
		if (!ensure(Pawn && LeePS))
		{
			return;
		}

		if (ALeePlayerController* LyraPC = GetController<ALeePlayerController>())
		{
			if (Pawn->InputComponent != nullptr)
			{
				InitializePlayerInput(Pawn->InputComponent);
			}
		}
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const ULeePawnData* PawnData = nullptr;
		if (ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnData = PawnExtComp->GetPawnData<ULeePawnData>();
		}

		if (bIsLocallyControlled && PawnData)
		{
			if (ULeeCameraComponent* CameraComp = ULeeCameraComponent::FindCameraComponent(Pawn))
			{
				CameraComp->DetermineCameraModeDelegate.BindUObject(this, &ThisClass::DetermineCameraMode);
			}
		}
		
	}

}

void ULeeHeroComponent::CheckDefaultInitialization()
{

	const FGameplayTag& InitTags = FGameplayTag();
	static const TArray<FGameplayTag> StateChain =
	{
		MyTags::InitState::Spawned,
		MyTags::InitState::DataAvailable,
		MyTags::InitState::DataInitialized,
		MyTags::InitState::GameplayReady
	};

	ContinueInitStateChain(StateChain);
}

TSubclassOf<class ULeeCameraMode> ULeeHeroComponent::DetermineCameraMode() const
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return nullptr;
	}

	if (ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const ULeePawnData* PawnData = PawnExtComp->GetPawnData<ULeePawnData>())
		{
			return PawnData->DefaultCameraMode;
		}
	}
	return nullptr;
}

void ULeeHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	Subsystem->ClearAllMappings();
	if (const ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const ULeePawnData* PawnData = PawnExtComp->GetPawnData<ULeePawnData>())
		{
			if (const ULeeInputConfig* InputConfig = PawnData->InputConfig)
			{
				const FGameplayTag& GameplayTags = FGameplayTag();

				for (const FLeeMappableConfigPair& Pair : DefaultInputConfigs)
				{
					if (Pair.bShouldActivateAutomatically)
					{
						FModifyContextOptions Options = {};
						Options.bIgnoreAllPressedKeysUntilRelease = false;

						// 5.6: UPlayerMappableInputConfig 대신 MappingContexts 직접 사용
						for (const FInputMappingContextAndPriority& MappingPair : Pair.GetMappingContexts())
						{
							if (MappingPair.InputMapping)
							{
								Subsystem->AddMappingContext(MappingPair.InputMapping, MappingPair.Priority, Options);
							}
						}
					}
				} // DefaultInputConfigs
				// Cast로 변경: B_SimpleHeroPawn 등 ULeeInputComponent가 아닌 Pawn에서도 크래시하지 않도록
				if (ULeeInputComponent* LeeIC = Cast<ULeeInputComponent>(PlayerInputComponent))
				{
					LeeIC->BindNativeAction(InputConfig, MyTags::Lyra::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move,false);
					LeeIC->BindNativeAction(InputConfig, MyTags::Lyra::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse,false);
				}
			}
		}
	}
}

void ULeeHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.f, Controller->GetControlRotation().Yaw,0.f );

		if (Value.X != 0.f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);

			Pawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			Pawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void ULeeHeroComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}
	const FVector2D Value = InputActionValue.Get<FVector2D>();
	if (Value.X != 0.f)
	{
		Pawn->AddControllerYawInput(Value.X);
	}
	if (Value.Y != 0.f)
	{
		double AimInversionValue = -Value.Y;
		Pawn->AddControllerPitchInput(AimInversionValue);
	}
}



























