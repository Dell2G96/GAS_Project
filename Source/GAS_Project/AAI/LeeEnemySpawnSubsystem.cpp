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
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AMessage/LeeVerbMessage.h"
#include "GAS_Project/ASaveGame/LeeSaveSubsystem.h"

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

	// 26.10.30 추가 - 정예 몹 처치 기록용 사망 메시지 구독
	EliminationListenerHandle = UGameplayMessageSubsystem::Get(&InWorld).RegisterListener(
		MyTags::Lyra::Lyra_Elimination_Message, this, &ThisClass::HandleEliminationMessage);
}

// 메시지 구독 해제
void ULeeEnemySpawnSubsystem::Deinitialize()
{
	EliminationListenerHandle.Unregister();

	Super::Deinitialize();
}

// 정예 몹 자리이고 세이브에 처치 기록이 있으면 true (일반 몹 자리는 항상 false)
bool ULeeEnemySpawnSubsystem::IsSpawnPointDefeated(const ALeeEnemySpawnPoint* SpawnPoint) const
{
	if (!SpawnPoint || SpawnPoint->ShouldRespawnOnReset())
	{
		return false;
	}

	const ULeeSaveSubsystem* SaveSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<ULeeSaveSubsystem>();
	return SaveSubsystem && SaveSubsystem->IsSpawnPointDefeated(GetWorld(), SpawnPoint->GetSpawnPointId());
}

// 레벨의 모든 스폰 지점을 돌며 몹을 생성한다 (26.10.30 수정 - 처치된 정예 몹 자리는 건너뜀)
void ULeeEnemySpawnSubsystem::OnExperienceLoaded(const ULeeExperienceDefinition* Experience)
{
	ValidateSpawnPointIds();

	for (TActorIterator<ALeeEnemySpawnPoint> It(GetWorld()); It; ++It)
	{
		if (!IsSpawnPointDefeated(*It))
		{
			SpawnEnemy(*It);
		}
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

// 죽은 대상이 정예 몹 스폰 지점의 봇이면 그 SpawnPointId를 세이브에 기록한다 (기록과 동시에 디스크 저장)
void ULeeEnemySpawnSubsystem::HandleEliminationMessage(FGameplayTag Channel, const FLeeVerbMessage& Message)
{
	const APlayerState* DeadPlayerState = Cast<APlayerState>(Message.Target);
	const ALeePlayerBotController* BotController = DeadPlayerState ? Cast<ALeePlayerBotController>(DeadPlayerState->GetOwningController()) : nullptr;
	const ALeeEnemySpawnPoint* SpawnPoint = BotController ? BotController->HomeSpawnPoint.Get() : nullptr;
	if (!SpawnPoint || SpawnPoint->ShouldRespawnOnReset())
	{
		return;
	}

	if (ULeeSaveSubsystem* SaveSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<ULeeSaveSubsystem>())
	{
		SaveSubsystem->RecordSpawnPointDefeated(GetWorld(), SpawnPoint->GetSpawnPointId());
	}
}

// 정예 몹 자리의 SpawnPointId가 비어 있거나, 같은 ID를 쓰는 스폰 지점이 있으면 경고 로그를 남긴다
void ULeeEnemySpawnSubsystem::ValidateSpawnPointIds() const
{
	TMap<FName, const ALeeEnemySpawnPoint*> UsedIds;
	for (TActorIterator<ALeeEnemySpawnPoint> It(GetWorld()); It; ++It)
	{
		const ALeeEnemySpawnPoint* SpawnPoint = *It;
		const FName SpawnPointId = SpawnPoint->GetSpawnPointId();
		if (SpawnPointId.IsNone())
		{
			if (!SpawnPoint->ShouldRespawnOnReset())
			{
				UE_LOG(LogLee, Warning, TEXT("[EnemySpawn] %s: 정예 몹(bRespawnOnReset = false)인데 SpawnPointId가 비어 있음. 처치 기록이 저장되지 않음"), *GetNameSafe(SpawnPoint));
			}
			continue;
		}

		if (const ALeeEnemySpawnPoint** Existing = UsedIds.Find(SpawnPointId))
		{
			UE_LOG(LogLee, Warning, TEXT("[EnemySpawn] SpawnPointId [%s]가 %s와 %s에서 겹침."), *SpawnPointId.ToString(), *GetNameSafe(*Existing), *GetNameSafe(SpawnPoint));
			continue;
		}
		UsedIds.Add(SpawnPointId, SpawnPoint);
	}
}
