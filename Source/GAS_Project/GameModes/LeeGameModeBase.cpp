// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameModeBase.h"

#include "GameMapsSettings.h"
#include "LeeExperienceDefinition.h"
#include "LeeWorldSetting.h"
#include "LeeExperienceManagerComponent.h"
#include "LeeGameState.h"
#include "GameFramework/GameSession.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/AAI/LeePlayerBotController.h"
#include "GAS_Project/ACharacter/LeeCharacter.h"
#include "GAS_Project/APlayer/LeePlayerController.h"
#include "GAS_Project/APlayer/LeePlayerState.h"
#include "GAS_Project/ACharacter/LeePawnData.h"
#include "GAS_Project/ACharacter/LeePawnExtensionComponent.h"
#include "GAS_Project/AUI/LeeHUD.h"
#include "GAS_Project/System/LeeAssetManager.h"
#include "Kismet/GameplayStatics.h"

ALeeGameModeBase::ALeeGameModeBase(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)

{
	GameStateClass = ALeeGameState::StaticClass();
	GameSessionClass = ALeeGameState::StaticClass();
	PlayerControllerClass = ALeePlayerController::StaticClass();
	
	PlayerStateClass = ALeePlayerState::StaticClass();
	DefaultPawnClass = ALeeCharacter::StaticClass();
	HUDClass = ALeeHUD::StaticClass();
}

void ALeeGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);


	// 아직 게임인스턴스를 통해 초기화 작업이 진행되므로
	// 현 프레임에서는 Lyra의 Experience 를 처리 할 수 없음
	// 따라서 한 프레임 뒤에서 이벤트를 받아서 처리
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleMatchAssignmentIfNotExpectingOne);
}

void ALeeGameModeBase::InitGameState()
{
	Super::InitGameState();

	ULeeExperienceManagerComponent* ExperienceManagerComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
	check(ExperienceManagerComponent);

	ExperienceManagerComponent->CallOrRegister_OnExperienceLoaded(FOnLeeExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
	
}

UClass* ALeeGameModeBase::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const ULeePawnData* PawnData = GetPawnDataForController(InController))
	{
		if (PawnData->PawnClass)
		{
			return PawnData->PawnClass;
		}
	}
	
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ALeeGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (IsExperienceLoaded())
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
}

APawn* ALeeGameModeBase::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer,
	const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;
	SpawnInfo.bDeferConstruction = true;

	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo))
		{
			if (ULeePawnExtensionComponent* PawnExtComp = ULeePawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn))
			{
				if (const ULeePawnData* PawnData = GetPawnDataForController(NewPlayer))
				{
					PawnExtComp->SetPawnData(PawnData);
				}
			}
			SpawnedPawn->FinishSpawning(SpawnTransform);
			return SpawnedPawn;
		}
	}
	return nullptr;
}

bool ALeeGameModeBase::ShouldSpawnAtStartSpot(AController* Player)
{
	return Super::ShouldSpawnAtStartSpot(Player);
}

AActor* ALeeGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	return Super::ChoosePlayerStart_Implementation(Player);
}

void ALeeGameModeBase::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

bool ALeeGameModeBase::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return Super::PlayerCanRestart_Implementation(Player);
}

bool ALeeGameModeBase::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	return Super::UpdatePlayerStartSpot(Player, Portal, OutErrorMessage);
}

void ALeeGameModeBase::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);
}

void ALeeGameModeBase::FailedToRestartPlayer(AController* NewPlayer)
{
	Super::FailedToRestartPlayer(NewPlayer);
}

void ALeeGameModeBase::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
}

bool ALeeGameModeBase::ControllerCanRestart(AController* Controller)
{
	return false;
}


void ALeeGameModeBase::OnMatchAssignmentGiven(FPrimaryAssetId ExperienceId, const FString& ExperienceIdSource)
{
}

bool ALeeGameModeBase::TryDedicatedServerLogin()
{
	// // Some basic code to register as an active dedicated server, this would be heavily modified by the game
	// FString DefaultMap = UGameMapsSettings::GetGameDefaultMap();
	// UWorld* World = GetWorld();
	// UGameInstance* GameInstance = GetGameInstance();
	// if (GameInstance && World && World->GetNetMode() == NM_DedicatedServer && World->URL.Map == DefaultMap)
	// {
	// 	// Only register if this is the default map on a dedicated server
	// 	// UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();
	//
	// 	// Dedicated servers may need to do an online login
	// 	UserSubsystem->OnUserInitializeComplete.AddDynamic(this, &ALyraGameMode::OnUserInitializedForDedicatedServer);
	//
	// 	// There are no local users on dedicated server, but index 0 means the default platform user which is handled by the online login code
	// 	if (!UserSubsystem->TryToLoginForOnlinePlay(0))
	// 	{
	// 		OnUserInitializedForDedicatedServer(nullptr, false, FText(), ECommonUserPrivilege::CanPlayOnline, ECommonUserOnlineContext::Default);
	// 	}
	//
	// 	return true;
	// }

	return false;
}

// void ALeeGameModeBase::HostDedicatedServerMatch(ECommonSessionOnlineMode OnlineMode)
// {
// }
//
// void ALeeGameModeBase::OnUserInitializedForDedicatedServer(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error,
// 	ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
// {
// }

void ALeeGameModeBase::HandleMatchAssignmentIfNotExpectingOne()
{
	FPrimaryAssetId ExperienceId;
	FString ExperienceIdSource;
	
	UWorld* World = GetWorld();

	// 1) URL Options에서 "Experience" 파라미터 확인
	if (!ExperienceId.IsValid() && UGameplayStatics::HasOption(OptionsString, TEXT("Experience")))
	{
		const FString ExperienceFromOptions = UGameplayStatics::ParseOption(OptionsString, TEXT("Experience"));
		ExperienceId = FPrimaryAssetId(FName(FPrimaryAssetType((*ULeeExperienceDefinition::StaticClass()->GetName()))),FName(*ExperienceFromOptions));
		
		ExperienceIdSource = TEXT("OptionsString");
	}

	// if (!ExperienceId.IsValid() && World->IsPlayInEditor())
	// {
	// 	ExperienceId = GetDefault<ULeeDeveloperSettings>()->ExperienceOverride;
	// 	ExperienceIdSource = TEXT("DeveloperSettings");
	// }

	// 2) WorldSettings에서 DefaultGameplayExperience 확인
	if (!ExperienceId.IsValid())
	{
		if (ALeeWorldSetting* TypedWorldSettings = Cast<ALeeWorldSetting>(World->GetWorldSettings()))
		{
			ExperienceId = TypedWorldSettings->GetDefaultGameplayExperience();
			ExperienceIdSource = TEXT("WorldSettings");
		}
	}

	// 3) 최종 폴백: B_DefaultExperience
	if (!ExperienceId.IsValid()) 
	{
		if (TryDedicatedServerLogin())
		{
			// This will start to host as a dedicated server
			return;
		}
		
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType("LeeExperienceDefinition"), FName("B_DefaultExperience"));
		ExperienceIdSource = TEXT("Default");
	}
	OnMatchAssignmentGiven(ExperienceId);
}

void ALeeGameModeBase::OnMatchAssignmentGiven(const FPrimaryAssetId& ExperienceId)
{
	check(ExperienceId.IsValid());

	ULeeExperienceManagerComponent* ExperienceManagerComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
	check(ExperienceManagerComponent);
	ExperienceManagerComponent->ServerSetCurrentExperience(ExperienceId);
}

bool ALeeGameModeBase::IsExperienceLoaded() const
{
	check(GameState);
	ULeeExperienceManagerComponent* ExperienceManagerComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
	check(ExperienceManagerComponent);
	
	return ExperienceManagerComponent->IsExperienceLoaded();
}

void ALeeGameModeBase::OnExperienceLoaded(const ULeeExperienceDefinition* CurrentExperience)
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Cast<APlayerController>(*Iterator);

		if (PC && PC->GetPawn() == nullptr)
		{
			if (PlayerCanRestart(PC))
			{
				RestartPlayer(PC);
			}
		}
	}
}

const class ULeePawnData* ALeeGameModeBase::GetPawnDataForController(const AController* InController) const
{
	if (InController)
	{
		if (const ALeePlayerState* LeePS = InController->GetPlayerState<ALeePlayerState>())
		{
			if (const ULeePawnData* PawnData = LeePS->GetPawnData<ULeePawnData>())
			{
				return PawnData;
			}
		}
	}
	check(GameState);
	ULeeExperienceManagerComponent* ExperienceManagerComponent = GameState->FindComponentByClass<ULeeExperienceManagerComponent>();
	check(ExperienceManagerComponent);

	if (ExperienceManagerComponent->IsExperienceLoaded())
	{
		const ULeeExperienceDefinition* Experience = ExperienceManagerComponent->GetCurrentExperienceChecked();

		// 26.03.19 추가
		if (InController->IsPlayerController())
		{
			return Experience->DefaultPawnData;
		}
		else if (const ALeePlayerBotController* BotController = Cast<ALeePlayerBotController>(InController))
		{
			const ULeePawnData* LeeEnemyPawnData = Experience->EnemyPawnClasses[BotController->BotIdentifier];
			if (LeeEnemyPawnData != nullptr)
			{
				return LeeEnemyPawnData;
			}
		}
		
		if (Experience->DefaultPawnData)
		{
			return Experience->DefaultPawnData;
		}

		
	}

	return nullptr;
}
