// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeBonfireSubsystem.h"

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
bool ULeeBonfireSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// 서버에서만 사망 메시지를 구독한다 (사망 메시지는 서버에서만 방송됨)
void ULeeBonfireSubsystem::OnWorldBeginPlay(UWorld& InWorld)
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
void ULeeBonfireSubsystem::Deinitialize()
{
	EliminationListenerHandle.Unregister();

	Super::Deinitialize();
}

// 사망한 대상이 플레이어면 YOU DIED를 보내고 RespawnDelay 뒤 부활시킨다 (Enemy 사망은 무시 → 리셋 때 정리)
void ULeeBonfireSubsystem::HandleEliminationMessage(FGameplayTag Channel, const FLeeVerbMessage& Message)
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

	FTimerHandle RespawnTimerHandle;
	const FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(
		this, &ThisClass::RespawnPlayer, TWeakObjectPtr<APlayerController>(PlayerController));
	GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, RespawnDelegate, RespawnDelay, false);
}

// 이전 Pawn(시체)을 정리하고 Enemy를 리셋한 뒤 플레이어를 다음 프레임에 재시작한다
void ULeeBonfireSubsystem::RespawnPlayer(TWeakObjectPtr<APlayerController> WeakPlayerController)
{
	PendingRespawnPlayers.Remove(WeakPlayerController);

	APlayerController* PlayerController = WeakPlayerController.Get();
	if (!PlayerController)
	{
		return;
	}

	ALeeGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ALeeGameModeBase>();
	if (!GameMode)
	{
		UE_LOG(LogLee, Warning, TEXT("[BonfireSubsystem] RespawnPlayer: ALeeGameModeBase가 아니라서 부활할 수 없음"));
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
void ULeeBonfireSubsystem::ResetEnemies()
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
void ULeeBonfireSubsystem::SendMessageToPlayer(APlayerController* PlayerController, FGameplayTag MessageTag) const
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
void ULeeBonfireSubsystem::DestroyPawnForReset(APawn* Pawn)
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
