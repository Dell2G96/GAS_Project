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
#include "GAS_Project/APlayer/LeePlayerSpawningManagerComponent.h"
#include "GAS_Project/System/LeeGameSession.h"
#include "GAS_Project/ACharacter/LeePawnData.h"
#include "GAS_Project/ACharacter/LeePawnExtensionComponent.h"
#include "GAS_Project/AUI/LeeHUD.h"
#include "GAS_Project/System/LeeAssetManager.h"
#include "Kismet/GameplayStatics.h"

ALeeGameModeBase::ALeeGameModeBase(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)

{
	GameStateClass = ALeeGameState::StaticClass();
	GameSessionClass = ALeeGameSession::StaticClass();
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

// 캐싱된 StartSpot을 재사용하지 않고, 스폰할 때마다 ChoosePlayerStart로 위치를 다시 고르게 한다
bool ALeeGameModeBase::ShouldSpawnAtStartSpot(AController* Player)
{
	return false;
}

// 스폰 위치 선택을 SpawningManagerComponent에 위임 (없으면 엔진 기본 동작)
AActor* ALeeGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	if (ULeePlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<ULeePlayerSpawningManagerComponent>())
	{
		return PlayerSpawningComponent->ChoosePlayerStart(Player);
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

// 스폰 완료 후 처리를 SpawningManagerComponent에 먼저 알리고 엔진 기본 처리 진행
void ALeeGameModeBase::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	if (ULeePlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<ULeePlayerSpawningManagerComponent>())
	{
		PlayerSpawningComponent->FinishRestartPlayer(NewPlayer, StartRotation);
	}

	Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

// 플레이어 재시작 가능 여부를 Player/Bot 공용 함수 ControllerCanRestart로 판단
bool ALeeGameModeBase::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return ControllerCanRestart(Player);
}

bool ALeeGameModeBase::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	return Super::UpdatePlayerStartSpot(Player, Portal, OutErrorMessage);
}

void ALeeGameModeBase::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);
}

// 스폰에 실패하면 Pawn 클래스가 있고 재시작이 가능한 경우 다음 프레임에 다시 시도
void ALeeGameModeBase::FailedToRestartPlayer(AController* NewPlayer)
{
	Super::FailedToRestartPlayer(NewPlayer);

	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APlayerController* NewPC = Cast<APlayerController>(NewPlayer))
		{
			if (PlayerCanRestart(NewPC))
			{
				RequestPlayerRestartNextFrame(NewPlayer, false);
			}
			else
			{
				UE_LOG(LogLee, Verbose, TEXT("FailedToRestartPlayer(%s) and PlayerCanRestart returned false, so we're not going to try again."), *GetPathNameSafe(NewPlayer));
			}
		}
		else
		{
			RequestPlayerRestartNextFrame(NewPlayer, false);
		}
	}
	else
	{
		UE_LOG(LogLee, Verbose, TEXT("FailedToRestartPlayer(%s) but there's no pawn class so giving up."), *GetPathNameSafe(NewPlayer));
	}
}

// 다음 프레임에 Player 또는 Bot을 재시작(리스폰) 요청. bForceReset이면 현재 Pawn을 버리고 즉시 리셋
void ALeeGameModeBase::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
	if (bForceReset && (Controller != nullptr))
	{
		Controller->Reset();
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(PC, &APlayerController::ServerRestartPlayer_Implementation);
	}
	else if (ALeePlayerBotController* BotController = Cast<ALeePlayerBotController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(BotController, &ALeePlayerBotController::ServerRestartController);
	}
}

// Player/Bot 공용 재시작 가능 판단. 기본 검사 후 최종 규칙은 SpawningManagerComponent에 위임
bool ALeeGameModeBase::ControllerCanRestart(AController* Controller)
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		// this->PlayerCanRestart를 부르면 다시 이 함수로 돌아오므로 반드시 Super로 호출
		if (!Super::PlayerCanRestart_Implementation(PC))
		{
			return false;
		}
	}
	else
	{
		// Bot용 PlayerCanRestart 기본 검사
		if ((Controller == nullptr) || Controller->IsPendingKillPending())
		{
			return false;
		}
	}

	if (ULeePlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<ULeePlayerSpawningManagerComponent>())
	{
		return PlayerSpawningComponent->ControllerCanRestart(Controller);
	}

	return true;
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
			// 26.10.29 추가 - Controller BP에 지정된 PawnData를 우선 사용 (새 스폰 지점 방식)
			if (BotController->EnemyPawnData)
			{
				return BotController->EnemyPawnData;
			}

			// 레거시: Experience의 EnemyPawnClasses[BotIdentifier] (26.10.29 수정 - 키가 없을 때 크래시 나지 않도록 Find 사용)
			if (const TObjectPtr<const ULeePawnData>* LeeEnemyPawnData = Experience->EnemyPawnClasses.Find(BotController->BotIdentifier))
			{
				if (*LeeEnemyPawnData != nullptr)
				{
					return *LeeEnemyPawnData;
				}
			}
		}
		
		if (Experience->DefaultPawnData)
		{
			return Experience->DefaultPawnData;
		}

		
	}

	return nullptr;
}
