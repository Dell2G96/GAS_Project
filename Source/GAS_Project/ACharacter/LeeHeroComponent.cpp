// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeHeroComponent.h"

#include "EnhancedInputSubsystems.h"
#include "LeePawnExtensionComponent.h"
#include "TimerManager.h"
#include "GAS_Project/LeeLogChannels.h"

#include "Components/GameFrameworkComponentManager.h"
#include "LeeGameplayTags.h"

#include "GAS_Project/MyTags.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/ACamera/LeeCameraComponent.h"
#include "GAS_Project/ACamera/LeeCameraMode.h"
#include "GAS_Project/ACharacter/LeeTargetLockComponent.h"
#include "GAS_Project/AInput/LeeInputComponent.h"
#include "GAS_Project/AInput/LeeMappableConfigPair.h"
#include "GAS_Project/APlayer/LeePlayerController.h"
#include "GAS_Project/APlayer/LeePlayerState.h"
#include "GAS_Project/GameFeatures/GameFeatureAction_AddInputContextMapping.h"
#include "UserSettings/EnhancedInputUserSettings.h"


const FName ULeeHeroComponent::NAME_ActorFeatureName("Hero");

const FName ULeeHeroComponent::NAME_BindInputsNow("BindInputsNow");

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
	// Pawn 소멸/UnPossess 시 어빌리티 카메라 모드가 남지 않도록 무조건 정리 (안전망)
	AbilityCameraMode = nullptr;
	AbilityCameraModeOwningSpecHandle = FGameplayAbilitySpecHandle();
	AbilityCameraFocusTarget = nullptr;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AbilityCameraModeTimeoutHandle);
	}

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
			UE_LOG(LogLee, Warning, TEXT("HeroComponent Spawned->DataAvailable Failed: LeePS is null"));
			return false;
		}
		return true;
	}

	if (CurrentState == MyTags::InitState::DataAvailable && DesiredState == MyTags::InitState::DataInitialized)
	{
		// PawnExtensionComponent가 DataInitialized될 때까지 기다림 (== 모든 Feature Component가 DataAvailable인 상태)
		bool bHasPS = LeePS != nullptr;
		bool bPawnExtReady = Manager->HasFeatureReachedInitState(Pawn, ULeePawnExtensionComponent::NAME_ActorFeatureName, MyTags::InitState::DataInitialized);
		
		if (!bHasPS || !bPawnExtReady)
		{
			UE_LOG(LogLee, Warning, TEXT("HeroComponent DataAvailable->DataInitialized Waiting... HasPS: %d, PawnExtReady: %d"), bHasPS, bPawnExtReady);
		}
		
		return bHasPS && bPawnExtReady;
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
		
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		
		const ULeePawnData* PawnData = nullptr;
		
		if (ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnData = PawnExtComp->GetPawnData<ULeePawnData>();
			PawnExtComp->InitializeAbilitySystem(LeePS->GetLeeAbilitySystemComponent(), LeePS);
			
		}

		if (ALeePlayerController* LeePC = GetController<ALeePlayerController>())
		{
			if (Pawn->InputComponent != nullptr)
			{
				InitializePlayerInput(Pawn->InputComponent);
			}
		}

		if (PawnData)
		{
			if (ULeeCameraComponent* CameraComponent = ULeeCameraComponent::FindCameraComponent(Pawn))
			{
				CameraComponent->DetermineCameraModeDelegate.BindUObject(this, &ThisClass::DetermineCameraMode);
			}
		}
		
		// if (bIsLocallyControlled && PawnData)
		// {
		// 	if (ULeeCameraComponent* CameraComp = ULeeCameraComponent::FindCameraComponent(Pawn))
		// 	{
		// 		CameraComp->DetermineCameraModeDelegate.BindUObject(this, &ThisClass::DetermineCameraMode);
		// 	}
		// }
		//
		// if (ALeePlayerController* LeePC = GetController<ALeePlayerController>())
		// {
		// 	if (Pawn->InputComponent != nullptr)
		// 	{
		// 		InitializePlayerInput(Pawn->InputComponent);
		// 	}
		// }
		
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
	// 어빌리티가 임시 카메라 모드를 설정했다면 그것이 최우선 (처형/암살 시네마틱 등).
	// Clear되면 다음 프레임부터 아래 우선순위로 자연 복귀한다.
	if (AbilityCameraMode)
	{
		return AbilityCameraMode;
	}

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return nullptr;
	}

	// 2순위: 타겟 락온 중이면 락온 카메라 (긴 수명 모드 — AbilityCameraMode 슬롯을 쓰지 않으므로
	// 피니셔 시작/종료와 슬롯 경합 없이, 피니셔 종료 시 자동으로 이 우선순위로 복귀한다)
	if (const ULeeTargetLockComponent* TargetLock = ULeeTargetLockComponent::FindTargetLockComponent(Pawn))
	{
		if (TargetLock->IsLocked() && TargetLock->LockOnCameraMode)
		{
			return TargetLock->LockOnCameraMode;
		}
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

void ULeeHeroComponent::SetAbilityCameraMode(TSubclassOf<ULeeCameraMode> CameraMode, const FGameplayAbilitySpecHandle& OwningSpecHandle, AActor* FocusTarget, float MaxDuration)
{
	if (!CameraMode)
	{
		return;
	}

	AbilityCameraMode = CameraMode;
	AbilityCameraModeOwningSpecHandle = OwningSpecHandle;
	AbilityCameraFocusTarget = FocusTarget;

	// 안전망: EndAbility 경유 Clear가 어떤 이유로든 누락돼도 시간 만료로 기본 카메라 복귀
	// (무적 GE의 Duration 안전망과 동일한 사상 — 로직 버그가 있어도 시간이 해결한다)
	if (MaxDuration > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				AbilityCameraModeTimeoutHandle,
				FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					ClearAbilityCameraMode(AbilityCameraModeOwningSpecHandle);
				}),
				MaxDuration, /*bLoop*/false);
		}
	}
}

void ULeeHeroComponent::ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	// 설정한 어빌리티만 해제 가능 — 이후 다른 어빌리티가 카메라 모드를 쓰게 되어도 서로 지우지 않는다
	if (AbilityCameraModeOwningSpecHandle == OwningSpecHandle)
	{
		AbilityCameraMode = nullptr;
		AbilityCameraModeOwningSpecHandle = FGameplayAbilitySpecHandle();
		AbilityCameraFocusTarget = nullptr;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(AbilityCameraModeTimeoutHandle);
		}
	}
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

	//Subsystem->ClearAllMappings();
	if (const ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const ULeePawnData* PawnData = PawnExtComp->GetPawnData<ULeePawnData>())
		{
			if (const ULeeInputConfig* InputConfig = PawnData->InputConfig)
			{
				const FGameplayTag& GameplayTags = FGameplayTag();
				
				for (const FInputMappingContextAndPriority& Mapping : DefaultInputMappings)
				{
					if (UInputMappingContext* IMC = Mapping.InputMapping.LoadSynchronous())
					{
						if (Mapping.bRegisterWithSettings)
						{
							if (UEnhancedInputUserSettings* Settings = Subsystem->GetUserSettings())
							{
								Settings->RegisterInputMappingContext(IMC);

								FModifyContextOptions Options = {};
								Options.bIgnoreAllPressedKeysUntilRelease = false;

								Subsystem->AddMappingContext(IMC,Mapping.Priority,Options);
							}
						}
						FModifyContextOptions Options = {};
						Options.bIgnoreAllPressedKeysUntilRelease = false;

						Subsystem->AddMappingContext(IMC,Mapping.Priority,Options);
					}
				}
				if (ULeeInputComponent* LeeIC = Cast<ULeeInputComponent>(PlayerInputComponent))
				{
					{
						LeeIC->AddInputMappings(InputConfig,Subsystem);
						TArray<uint32> BindHandles;
						//LeeIC->BindAbilityAction();
						LeeIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, BindHandles);
					
						LeeIC->BindNativeAction(InputConfig, MyTags::Lyra::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move,false);
						LeeIC->BindNativeAction(InputConfig, MyTags::Lyra::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse,false);
						LeeIC->BindNativeAction(InputConfig, MyTags::Souls::InputTag_TargetLock_SwitchLeft, ETriggerEvent::Triggered, this, &ThisClass::Input_TargetLockSwitchLeft,false);
						LeeIC->BindNativeAction(InputConfig, MyTags::Souls::InputTag_TargetLock_SwitchRight, ETriggerEvent::Triggered, this, &ThisClass::Input_TargetLockSwitchRight,false);
					}
				}
			}
		}
	}
	if (ensure(!bReadyToBindInputs))
	{
		bReadyToBindInputs = true;
	}
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APawn*>(Pawn), NAME_BindInputsNow);
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

	// 락온 중에는 락온 카메라가 ControlRotation을 전담 구동하므로 자유 시점 입력을 차단한다
	// (클래식 소울라이크 방식 — 완전 고정). 전환은 Input_TargetLockSwitchLeft/Right가 별도 처리
	if (const ULeeTargetLockComponent* TargetLock = ULeeTargetLockComponent::FindTargetLockComponent(Pawn))
	{
		if (TargetLock->IsLocked())
		{
			return;
		}
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

void ULeeHeroComponent::Input_TargetLockSwitchLeft(const FInputActionValue& InputActionValue)
{
	if (ULeeTargetLockComponent* TargetLock = ULeeTargetLockComponent::FindTargetLockComponent(GetPawn<APawn>()))
	{
		TargetLock->SwitchTarget(/*bWantRight*/false);
	}
}

void ULeeHeroComponent::Input_TargetLockSwitchRight(const FInputActionValue& InputActionValue)
{
	if (ULeeTargetLockComponent* TargetLock = ULeeTargetLockComponent::FindTargetLockComponent(GetPawn<APawn>()))
	{
		TargetLock->SwitchTarget(/*bWantRight*/true);
	}
}

void ULeeHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (ULeeAbilitySystemComponent* LeeASC = PawnExtComp->GetLeeAbilitySystemComponent())
			{
 				LeeASC->AbilityInputTagPressed(InputTag);
			}
			
		}
	}
}

void ULeeHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (ULeeAbilitySystemComponent* LeeASC = PawnExtComp->GetLeeAbilitySystemComponent())
			{
				LeeASC->AbilityInputTagReleased(InputTag);
			}
			
		}
	}
}