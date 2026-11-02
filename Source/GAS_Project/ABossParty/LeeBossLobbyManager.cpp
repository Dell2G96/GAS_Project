// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeBossLobbyManager.h"

#include "CommonActivatableWidget.h"
#include "CommonSessionSubsystem.h"
#include "CommonUIExtensions.h"
#include "EngineUtils.h"
#include "LeeBossJoinBeaconHostObject.h"
#include "OnlineBeaconHost.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/GameModes/LeeExperienceDefinition.h"
#include "GAS_Project/GameModes/LeeExperienceManagerComponent.h"

// 모든 클라이언트에 항상 복제 (위치와 무관한 로비 상태 Actor)
ALeeBossLobbyManager::ALeeBossLobbyManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);
}

// 월드에서 첫 번째 로비 관리 Actor를 찾는다 (맵에 1개만 배치)
ALeeBossLobbyManager* ALeeBossLobbyManager::Get(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ALeeBossLobbyManager> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

// 준비 목록과 출발 시각 복제
void ALeeBossLobbyManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ReadyPlayers);
	DOREPLIFETIME(ThisClass, CountdownEndTime);
	DOREPLIFETIME(ThisClass, ArrivingPlayerCount);
}

// 서버: 입장 요청 HostObject 생성·등록 + 접속/퇴장 구독 / 로컬 플레이어: 로비 HUD 표시
void ALeeBossLobbyManager::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		JoinHostObject = GetWorld()->SpawnActor<ALeeBossJoinBeaconHostObject>();
		if (JoinHostObject)
		{
			JoinHostObject->OnRequestApproved.AddUObject(this, &ThisClass::HandleJoinApproved);
		}
		TryRegisterJoinHost();

		PostLoginHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &ThisClass::HandlePostLogin);
		LogoutHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &ThisClass::HandleLogout);
	}

	if (GetNetMode() != NM_DedicatedServer)
	{
		TryShowLobbyHUD();
	}
}

// 등록 해제, HUD 닫기, 타이머·구독 정리
void ALeeBossLobbyManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RegisterRetryTimerHandle);
	GetWorldTimerManager().ClearTimer(ShowHUDRetryTimerHandle);
	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	GetWorldTimerManager().ClearTimer(ArrivalTimeoutTimerHandle);

	FGameModeEvents::GameModePostLoginEvent.Remove(PostLoginHandle);
	FGameModeEvents::GameModeLogoutEvent.Remove(LogoutHandle);

	if (JoinHostObject)
	{
		const UCommonSessionSubsystem* SessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCommonSessionSubsystem>() : nullptr;
		if (AOnlineBeaconHost* Listener = SessionSubsystem ? SessionSubsystem->GetBeaconHostListener() : nullptr)
		{
			Listener->UnregisterHost(JoinHostObject->GetBeaconType());
		}
		JoinHostObject->Destroy();
		JoinHostObject = nullptr;
	}

	if (UCommonActivatableWidget* Widget = LobbyHUDWidget.Get())
	{
		UCommonUIExtensions::PopContentFromLayer(Widget);
	}

	Super::EndPlay(EndPlayReason);
}

// 세션 비콘 리스너는 맵 로드 직후 CommonSession이 다시 만든다 — 아직 없으면 0.5초 뒤 재시도
void ALeeBossLobbyManager::TryRegisterJoinHost()
{
	const UCommonSessionSubsystem* SessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCommonSessionSubsystem>() : nullptr;
	AOnlineBeaconHost* Listener = SessionSubsystem ? SessionSubsystem->GetBeaconHostListener() : nullptr;
	if (!Listener)
	{
		GetWorldTimerManager().SetTimer(RegisterRetryTimerHandle, this, &ThisClass::TryRegisterJoinHost, 0.5f, false);
		return;
	}

	if (!Listener->GetHost(JoinHostObject->GetBeaconType()))
	{
		Listener->RegisterHost(JoinHostObject);
		UE_LOG(LogLee, Log, TEXT("[BossLobby] 입장 요청 비콘 등록 완료"));
	}
}

// 로컬 플레이어의 Menu 레이어에 로비 HUD Push — PlayerController/UI 레이아웃이 아직 없으면 재시도
void ALeeBossLobbyManager::TryShowLobbyHUD()
{
	if (LobbyHUDWidget.IsValid())
	{
		return;
	}

	UClass* WidgetClass = LobbyHUDClass.LoadSynchronous();
	if (!WidgetClass)
	{
		UE_LOG(LogLee, Warning, TEXT("[BossLobby] LobbyHUDClass가 비어 있어 로비 HUD를 띄우지 않습니다."));
		return;
	}

	APlayerController* LocalController = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController(GetWorld()) : nullptr;
	if (LocalController && LocalController->GetLocalPlayer())
	{
		LobbyHUDWidget = UCommonUIExtensions::PushContentToLayer_ForPlayer(
			LocalController->GetLocalPlayer(), MyTags::Lyra::UI_Layer_Menu, WidgetClass);
	}

	if (!LobbyHUDWidget.IsValid())
	{
		GetWorldTimerManager().SetTimer(ShowHUDRetryTimerHandle, this, &ThisClass::TryShowLobbyHUD, 0.5f, false);
	}
}

// [서버] 준비 목록에 추가/제거 후 카운트다운 재판정
void ALeeBossLobbyManager::SetPlayerReady(APlayerState* PlayerState, bool bReady)
{
	if (!HasAuthority() || !PlayerState || bTravelStarted)
	{
		return;
	}

	if (bReady)
	{
		ReadyPlayers.AddUnique(PlayerState);
	}
	else
	{
		ReadyPlayers.Remove(PlayerState);
	}

	EvaluateCountdown();
}

bool ALeeBossLobbyManager::IsPlayerReady(const APlayerState* PlayerState) const
{
	return PlayerState && ReadyPlayers.Contains(PlayerState);
}

// 서버 월드 시간 기준으로 남은 시간 계산 (클라이언트도 GameState의 서버 시간으로 같은 값을 얻는다)
float ALeeBossLobbyManager::GetCountdownRemaining() const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (CountdownEndTime <= 0.0f || !GameState)
	{
		return -1.0f;
	}
	return FMath::Max(CountdownEndTime - GameState->GetServerWorldTimeSeconds(), 0.0f);
}

// 새 참가자는 아직 준비 전이므로 카운트다운이 취소된다
void ALeeBossLobbyManager::HandlePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	if (GameMode && GameMode->GetWorld() == GetWorld())
	{
		// 26.11.03 추가 - 도착한 게스트 본인의 접속 대기만 지운다
		// (시간 초과로 이미 빠진 게스트가 늦게 와도 다른 게스트의 대기는 남는다)
		const APlayerState* NewPlayerState = NewPlayer ? NewPlayer->PlayerState : nullptr;
		if (NewPlayerState && NewPlayerState->GetUniqueId().IsValid())
		{
			const FUniqueNetIdRepl& NewPlayerId = NewPlayerState->GetUniqueId();
			PendingArrivals.RemoveAll([&NewPlayerId](const FLeeBossArrival& Arrival) { return Arrival.PlayerId == NewPlayerId; });
			ArrivingPlayerCount = PendingArrivals.Num();
		}

		EvaluateCountdown();
	}
}

// 접속 대기 인원 추가 → 기한 타이머 갱신 → 카운트다운 재판정(진행 중이면 취소)
void ALeeBossLobbyManager::HandleJoinApproved(const FUniqueNetIdRepl& PlayerId)
{
	if (bTravelStarted)
	{
		return;
	}

	if (!PlayerId.IsValid())
	{
		// ID가 없으면 도착해도 대기를 지울 수 없어 30초 동안 출발만 막히므로 기록하지 않는다
		UE_LOG(LogLee, Warning, TEXT("[BossLobby] 수락한 요청자의 고유 ID가 없어 접속 대기로 기록하지 않습니다."));
		return;
	}

	// 같은 사람을 다시 수락했다면(시간 초과 후 재요청 등) 기존 기록을 지우고 기한을 새로 잡는다
	PendingArrivals.RemoveAll([&PlayerId](const FLeeBossArrival& Arrival) { return Arrival.PlayerId == PlayerId; });

	FLeeBossArrival Arrival;
	Arrival.PlayerId = PlayerId;
	Arrival.Deadline = GetWorld()->GetTimeSeconds() + ArrivalTimeoutSeconds;
	PendingArrivals.Add(Arrival);
	ArrivingPlayerCount = PendingArrivals.Num();

	RemoveExpiredArrivals();
	EvaluateCountdown();
}

// 기한이 지난 접속 대기 인원은 접속 실패로 보고 제거 → 남은 인원 중 가장 빠른 기한에 다시 확인
void ALeeBossLobbyManager::RemoveExpiredArrivals()
{
	const float Now = GetWorld()->GetTimeSeconds();
	const int32 RemovedCount = PendingArrivals.RemoveAll([Now](const FLeeBossArrival& Arrival) { return Arrival.Deadline <= Now; });
	ArrivingPlayerCount = PendingArrivals.Num();

	GetWorldTimerManager().ClearTimer(ArrivalTimeoutTimerHandle);
	if (PendingArrivals.Num() > 0)
	{
		// 수락한 순서대로 쌓이므로 0번이 가장 빠른 기한
		GetWorldTimerManager().SetTimer(ArrivalTimeoutTimerHandle, this, &ThisClass::RemoveExpiredArrivals,
			FMath::Max(PendingArrivals[0].Deadline - Now, 0.01f), false);
	}

	if (RemovedCount > 0)
	{
		UE_LOG(LogLee, Log, TEXT("[BossLobby] 수락한 게스트 %d명이 %.0f초 안에 접속하지 않아 대기에서 제외합니다."), RemovedCount, ArrivalTimeoutSeconds);
		EvaluateCountdown();
	}
}

// 퇴장한 플레이어 제거 — PlayerArray에서 빠진 뒤 판정하도록 다음 틱에 재판정
void ALeeBossLobbyManager::HandleLogout(AGameModeBase* GameMode, AController* Exiting)
{
	if (!GameMode || GameMode->GetWorld() != GetWorld())
	{
		return;
	}

	if (Exiting)
	{
		ReadyPlayers.Remove(Exiting->PlayerState);
	}
	GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::EvaluateCountdown);
}

// 호스트 포함 모든 플레이어가 준비면 카운트다운 시작, 한 명이라도 준비가 아니면 취소
void ALeeBossLobbyManager::EvaluateCountdown()
{
	AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!GameState || bTravelStarted)
	{
		return;
	}

	// 나간 플레이어 정리
	ReadyPlayers.RemoveAll([GameState](const TObjectPtr<APlayerState>& PlayerState)
	{
		return !PlayerState || !GameState->PlayerArray.Contains(PlayerState);
	});

	int32 PlayerCount = 0;
	bool bAllReady = true;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (!PlayerState || PlayerState->IsABot())
		{
			continue;
		}
		++PlayerCount;
		bAllReady &= ReadyPlayers.Contains(PlayerState);
	}
	bAllReady &= (PlayerCount > 0);

	// 26.11.03 추가 - 수락했지만 아직 접속 중인 게스트가 있으면 출발하지 않는다 (호스트 혼자 떠나는 문제)
	bAllReady &= PendingArrivals.IsEmpty();

	const bool bCounting = GetWorldTimerManager().IsTimerActive(CountdownTimerHandle);
	if (bAllReady && !bCounting)
	{
		CountdownEndTime = GameState->GetServerWorldTimeSeconds() + CountdownSeconds;
		GetWorldTimerManager().SetTimer(CountdownTimerHandle, this, &ThisClass::StartBossTravel, FMath::Max(CountdownSeconds, 0.01f), false);
	}
	else if (!bAllReady && bCounting)
	{
		GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
		CountdownEndTime = 0.0f;
	}
}

// 대기 중인 입장 요청을 거절하고, 로비 Experience의 BossMap으로 접속을 유지한 채 이동
void ALeeBossLobbyManager::StartBossTravel()
{
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	const ULeeExperienceManagerComponent* ExperienceComponent = GameState ? GameState->FindComponentByClass<ULeeExperienceManagerComponent>() : nullptr;
	const ULeeExperienceDefinition* Experience = (ExperienceComponent && ExperienceComponent->IsExperienceLoaded()) ? ExperienceComponent->GetCurrentExperienceChecked() : nullptr;
	const FString BossMapName = Experience ? Experience->BossMap.GetLongPackageName() : FString();
	if (BossMapName.IsEmpty())
	{
		UE_LOG(LogLee, Warning, TEXT("[BossLobby] 로비 Experience에 BossMap이 지정되지 않아 출발할 수 없습니다."));
		CountdownEndTime = 0.0f;

		// 26.11.03 추가 - 전원 준비 상태로 남으면 카운트다운이 다시 시작될 계기가 없으므로 준비를 모두 해제한다
		// (다시 준비를 누르면 EvaluateCountdown이 카운트다운을 새로 시작한다)
		ReadyPlayers.Empty();
		return;
	}

	bTravelStarted = true;

	if (JoinHostObject)
	{
		JoinHostObject->RejectAllRequests();
	}

	GetWorld()->ServerTravel(BossMapName);
}
