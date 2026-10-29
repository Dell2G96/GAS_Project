// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeEnemySpawnSubsystem.h"

#include "EngineUtils.h"
#include "LeeEnemySpawnPoint.h"
#include "LeePlayerBotController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/ACharacter/LeePawnExtensionComponent.h"
#include "GAS_Project/GameModes/LeeExperienceManagerComponent.h"
#include "GAS_Project/GameModes/LeeGameModeBase.h"

// 게임/PIE 월드에서만 생성한다 (에디터 프리뷰 월드 등 제외)
bool ULeeEnemySpawnSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// 서버에서만 Experience 로드 완료를 기다린다 (플레이어/PawnData 준비 후 생성하도록 LowPriority)
void ULeeEnemySpawnSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (InWorld.GetNetMode() == NM_Client)
	{
		return;
	}

	AGameStateBase* GameState = InWorld.GetGameState();
	ULeeExperienceManagerComponent* ExperienceComponent = GameState ? GameState->FindComponentByClass<ULeeExperienceManagerComponent>() : nullptr;
	if (!ExperienceComponent)
	{
		return;
	}

	ExperienceComponent->CallOrRegister_OnExperienceLoaded_LowPriority(
		FOnLeeExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

// 레벨의 모든 스폰 지점을 돌며 몹을 생성한다
void ULeeEnemySpawnSubsystem::OnExperienceLoaded(const ULeeExperienceDefinition* Experience)
{
	for (TActorIterator<ALeeEnemySpawnPoint> It(GetWorld()); It; ++It)
	{
		SpawnEnemy(*It);
	}
}

// 봇 Controller 생성 → 스폰 지점 기억(HomeSpawnPoint) → GameMode로 Pawn 생성 (기존 ULeeBotCreationComponent::SpawnOneBot과 같은 순서)
void ULeeEnemySpawnSubsystem::SpawnEnemy(ALeeEnemySpawnPoint* SpawnPoint)
{
	ALeeGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ALeeGameModeBase>();
	TSubclassOf<ALeePlayerBotController> ControllerClass = SpawnPoint ? SpawnPoint->GetBotControllerClass() : nullptr;
	if (!GameMode || !ControllerClass)
	{
		UE_LOG(LogLee, Warning, TEXT("[EnemySpawn] %s: BotControllerClass가 비어 있거나 GameMode가 ALeeGameModeBase가 아님"), *GetNameSafe(SpawnPoint));
		return;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.ObjectFlags |= RF_Transient;
	ALeePlayerBotController* BotController = GetWorld()->SpawnActor<ALeePlayerBotController>(ControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
	if (!BotController)
	{
		return;
	}

	// RestartPlayer → ChoosePlayerStart가 이 자리를 반환한다 (리셋 후에도 동일)
	BotController->HomeSpawnPoint = SpawnPoint;

	// 네임플레이트(W_Nameplate_Souls)가 PlayerState의 이름을 표시한다: 스폰 지점 덮어쓰기 → Controller BP 기본값 순
	const FText& DisplayName = SpawnPoint->GetDisplayNameOverride().IsEmpty() ? BotController->EnemyDisplayName : SpawnPoint->GetDisplayNameOverride();
	if (BotController->PlayerState && !DisplayName.IsEmpty())
	{
		BotController->PlayerState->SetPlayerName(DisplayName.ToString());
	}

	GameMode->GenericPlayerInitialization(BotController);
	GameMode->RestartPlayer(BotController);

	if (APawn* Pawn = BotController->GetPawn())
	{
		if (ULeePawnExtensionComponent* PawnExtensionComponent = ULeePawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnExtensionComponent->CheckDefaultInitialization();
		}
	}
}
