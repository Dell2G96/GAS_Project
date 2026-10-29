// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeSavePointSubsystem.h"

#include "LeeSavePointActor.h"
#include "GAS_Project/APlayer/LeePlayerStart.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AAI/LeePlayerBotController.h"
#include "GAS_Project/ACharacter/LeeCharacter.h"
#include "GAS_Project/ACharacter/LeeHealthComponent.h"
#include "GAS_Project/AMessage/LeeVerbMessage.h"
#include "GAS_Project/APlayer/LeePlayerState.h"
#include "GAS_Project/GameModes/LeeGameModeBase.h"

// 게임/PIE 월드에서만 생성한다 (에디터 프리뷰 월드 등 제외)
bool ULeeSavePointSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// 서버에서만 사망 메시지를 구독한다 (사망 메시지는 서버에서만 방송됨)
void ULeeSavePointSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (InWorld.GetNetMode() == NM_Client)
	{
		return;
	}

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(&InWorld);
	EliminationListenerHandle = MessageSystem.RegisterListener(
		MyTags::Lyra::Lyra_Elimination_Message, this, &ThisClass::HandleEliminationMessage);
}

// 메시지 구독 해제
void ULeeSavePointSubsystem::Deinitialize()
{
	EliminationListenerHandle.Unregister();

	Super::Deinitialize();
}

// 사망한 대상이 플레이어면 YOU DIED를 보내고 부활 대기 목록에 넣는다 (Enemy 사망은 무시 → 리셋 때 정리)
void ULeeSavePointSubsystem::HandleEliminationMessage(FGameplayTag Channel, const FLeeVerbMessage& Message)
{
	const APlayerState* DeadPlayerState = Cast<APlayerState>(Message.Target);
	if (!DeadPlayerState)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(DeadPlayerState->GetOwningController());
	if (!PlayerController || PendingRespawnPlayers.Contains(PlayerController))
	{
		return;
	}

	PendingRespawnPlayers.Add(PlayerController);
	SendMessageToPlayer(PlayerController, MyTags::Souls::Message_Death_YouDied);
}

// 부활 대기 중인 플레이어만 부활시킨다 (살아있는 플레이어의 요청·중복 요청은 무시)
void ULeeSavePointSubsystem::RequestRespawn(APlayerController* PlayerController)
{
	if (!PlayerController || !PendingRespawnPlayers.Contains(PlayerController))
	{
		return;
	}

	PendingRespawnPlayers.Remove(PlayerController);
	RespawnPlayer(PlayerController);
}

// SavePointId로 등록한다. ID가 비었거나 겹치면 경고만 남기고 등록하지 않는다
void ULeeSavePointSubsystem::RegisterSavePoint(ALeeSavePointActor* SavePoint)
{
	if (!SavePoint)
	{
		return;
	}

	const FName SavePointId = SavePoint->GetSavePointId();
	if (SavePointId.IsNone())
	{
		UE_LOG(LogLee, Warning, TEXT("[SavePointSubsystem] %s의 SavePointId가 비어 있음. 레벨에서 지정해주세요."), *GetNameSafe(SavePoint));
		return;
	}

	const TWeakObjectPtr<ALeeSavePointActor>* Existing = SavePoints.Find(SavePointId);
	if (Existing && Existing->IsValid() && Existing->Get() != SavePoint)
	{
		UE_LOG(LogLee, Warning, TEXT("[SavePointSubsystem] SavePointId [%s]가 %s와 %s에서 겹침."), *SavePointId.ToString(), *GetNameSafe(Existing->Get()), *GetNameSafe(SavePoint));
		return;
	}

	SavePoints.Add(SavePointId, SavePoint);
}

// 자기 자신으로 등록된 경우에만 제거한다
void ULeeSavePointSubsystem::UnregisterSavePoint(ALeeSavePointActor* SavePoint)
{
	if (SavePoint && FindSavePointById(SavePoint->GetSavePointId()) == SavePoint)
	{
		SavePoints.Remove(SavePoint->GetSavePointId());
	}
}

// SavePointId로 세이브 포인트을 찾는다
ALeeSavePointActor* ULeeSavePointSubsystem::FindSavePointById(FName SavePointId) const
{
	const TWeakObjectPtr<ALeeSavePointActor>* Found = SavePoints.Find(SavePointId);
	return Found ? Found->Get() : nullptr;
}

// 등록된 세이브 포인트 중 이 PlayerStart를 부활 지점으로 쓰는 곳이 있는지 확인한다
bool ULeeSavePointSubsystem::IsSavePointRespawnStart(const ALeePlayerStart* PlayerStart) const
{
	if (!PlayerStart)
	{
		return false;
	}

	for (const TPair<FName, TWeakObjectPtr<ALeeSavePointActor>>& Pair : SavePoints)
	{
		const ALeeSavePointActor* SavePoint = Pair.Value.Get();
		if (SavePoint && SavePoint->GetRespawnPlayerStart() == PlayerStart)
		{
			return true;
		}
	}
	return false;
}

// 이전 Pawn(시체)을 정리하고 Enemy를 리셋한 뒤 플레이어를 다음 프레임에 재시작한다
void ULeeSavePointSubsystem::RespawnPlayer(APlayerController* PlayerController)
{
	ALeeGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ALeeGameModeBase>();
	if (!GameMode)
	{
		UE_LOG(LogLee, Warning, TEXT("[SavePointSubsystem] RespawnPlayer: ALeeGameModeBase가 아니라서 부활할 수 없음"));
		return;
	}

	// 죽은 Pawn은 FinishDeath가 호출되지 않아 시체로 남아 있다 → 여기서 즉시 정리해 Controller를 비운다
	// (같은 프레임에 정리해야 다음 프레임 재시작 때 시체를 다시 빙의하지 않는다)
	DestroyPawnForReset(PlayerController->GetPawn());

	ResetEnemies();

	SendMessageToPlayer(PlayerController, MyTags::Souls::Message_Death_Respawn);
	GameMode->RequestPlayerRestartNextFrame(PlayerController, false);
}

// 모든 봇의 Pawn을 정리하고 다음 프레임에 재시작한다. 봇은 HomePlayerStart(처음 자리)에서 다시 생성된다
void ULeeSavePointSubsystem::ResetEnemies()
{
	UWorld* World = GetWorld();
	ALeeGameModeBase* GameMode = World ? World->GetAuthGameMode<ALeeGameModeBase>() : nullptr;
	if (!GameMode)
	{
		return;
	}

	for (TActorIterator<ALeePlayerBotController> It(World); It; ++It)
	{
		ALeePlayerBotController* BotController = *It;
		DestroyPawnForReset(BotController->GetPawn());
		GameMode->RequestPlayerRestartNextFrame(BotController, false);
	}
}

// 로컬 플레이어는 이 머신에서 바로 방송, 원격 플레이어는 PlayerState의 Client RPC로 전달
void ULeeSavePointSubsystem::SendMessageToPlayer(APlayerController* PlayerController, FGameplayTag MessageTag) const
{
	FLeeVerbMessage Message;
	Message.Verb = MessageTag;
	Message.Instigator = PlayerController;
	Message.Target = PlayerController->PlayerState;

	if (PlayerController->IsLocalController())
	{
		UGameplayMessageSubsystem::Get(PlayerController).BroadcastMessage(MessageTag, Message);
	}
	else if (ALeePlayerState* LeePlayerState = PlayerController->GetPlayerState<ALeePlayerState>())
	{
		LeePlayerState->ClientBroadcastMessage(Message);
	}
}

// 죽어 있으면 사망을 완료시켜(GA_Death 종료) 정리하고, ALeeCharacter면 ASC 연결 해제(장비 회수 포함) 후 파괴
void ULeeSavePointSubsystem::DestroyPawnForReset(APawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}

	// 사망 중인 Pawn은 FinishDeath가 호출된 적이 없다 → 여기서 완료시켜 GA_Death(Exclusive_Blocking)를 끝낸다
	if (ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(Pawn))
	{
		if (HealthComponent->GetDeathState() == ELeeDeathState::DeathStarted)
		{
			HealthComponent->FinishDeath();
		}
	}

	if (ALeeCharacter* LeeCharacter = Cast<ALeeCharacter>(Pawn))
	{
		LeeCharacter->UninitAndDestroy();
	}
	else
	{
		Pawn->Destroy();
	}
}
