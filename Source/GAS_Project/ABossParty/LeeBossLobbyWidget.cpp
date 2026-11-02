// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeBossLobbyWidget.h"

#include "CommonSessionSubsystem.h"
#include "CommonUserTypes.h"
#include "LeeBossExpeditionSubsystem.h"
#include "LeeBossJoinWaitWidget.h"
#include "LeeBossRequestActor.h"
#include "LeeBossRoomEntryWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"
#include "GAS_Project/GameModes/LeeUserFacingExperience.h"

// 세션에 광고하는 방 정보 키 (호스트가 쓰고, 검색하는 쪽이 읽는다)
static const FName NAME_LeeBossRoomName(TEXT("LEE_ROOMNAME"));
static const FName NAME_LeeBossBossName(TEXT("LEE_BOSSNAME"));

// 버튼 클릭 바인딩
void ULeeBossLobbyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CreateRoomButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCreateRoomClicked);
	RefreshButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRefreshClicked);
	CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);

	JoinWaitPanel->SetVisibility(ESlateVisibility::Collapsed);
}

// 창이 열릴 때 방 만들기/입장 결과 이벤트 구독
void ULeeBossLobbyWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (UCommonSessionSubsystem* SessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCommonSessionSubsystem>() : nullptr)
	{
		CreateSessionCompleteHandle = SessionSubsystem->OnCreateSessionCompleteEvent.AddUObject(this, &ThisClass::HandleCreateSessionComplete);
		JoinSessionCompleteHandle = SessionSubsystem->OnJoinSessionCompleteEvent.AddUObject(this, &ThisClass::HandleJoinSessionComplete);
	}
}

// 창이 닫힐 때 구독 해제
void ULeeBossLobbyWidget::NativeOnDeactivated()
{
	if (UCommonSessionSubsystem* SessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCommonSessionSubsystem>() : nullptr)
	{
		SessionSubsystem->OnCreateSessionCompleteEvent.Remove(CreateSessionCompleteHandle);
		SessionSubsystem->OnJoinSessionCompleteEvent.Remove(JoinSessionCompleteHandle);
	}

	Super::NativeOnDeactivated();
}

// 방 만들기 실패 시 이유 표시
void ULeeBossLobbyWidget::HandleCreateSessionComplete(const FOnlineResultInformation& Result)
{
	if (!Result.bWasSuccessful)
	{
		if (ULeeBossExpeditionSubsystem* ExpeditionSubsystem = ULeeBossExpeditionSubsystem::Get(this))
		{
			ExpeditionSubsystem->CancelExpedition();
		}
		SetStatusText(Result.ErrorText.IsEmpty()
			? NSLOCTEXT("LeeBossLobby", "CreateFailed", "방을 만들지 못했습니다")
			: FText::Format(NSLOCTEXT("LeeBossLobby", "CreateFailedReason", "방을 만들지 못했습니다: {0}"), Result.ErrorText));
	}
}

// 방 입장 실패 시 이유 표시
void ULeeBossLobbyWidget::HandleJoinSessionComplete(const FOnlineResultInformation& Result)
{
	if (!Result.bWasSuccessful)
	{
		// 26.11.02 추가 - 수락 후 참가에 실패하면 대기 패널을 닫고 필드에 남는다
		JoinWaitPanel->HideWaitPanel();
		if (ULeeBossExpeditionSubsystem* ExpeditionSubsystem = ULeeBossExpeditionSubsystem::Get(this))
		{
			ExpeditionSubsystem->CancelExpedition();
		}
		SetStatusText(Result.ErrorText.IsEmpty()
			? NSLOCTEXT("LeeBossLobby", "JoinFailed", "방에 입장하지 못했습니다. 새로고침 후 다시 시도하세요")
			: FText::Format(NSLOCTEXT("LeeBossLobby", "JoinFailedReason", "방에 입장하지 못했습니다: {0}"), Result.ErrorText));
	}
}

// 도움 요청 Actor 저장 후 방 목록 검색
void ULeeBossLobbyWidget::InitializeLobby(ALeeBossRequestActor* InRequestActor)
{
	RequestActor = InRequestActor;
	RefreshRoomList();
}

// 방 만들기: 로비 Experience로 호스팅 요청을 만들고 방 이름·보스 이름을 세션에 실어 리슨서버로 로비 맵 이동
void ULeeBossLobbyWidget::HandleCreateRoomClicked()
{
	const ALeeBossRequestActor* Actor = RequestActor.Get();
	const ULeeUserFacingExperience* LobbyExperience = Actor ? Actor->GetBossLobbyExperience() : nullptr;
	UCommonSessionSubsystem* SessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCommonSessionSubsystem>() : nullptr;
	if (!LobbyExperience || !SessionSubsystem)
	{
		SetStatusText(NSLOCTEXT("LeeBossLobby", "NoLobbySetting", "로비 설정이 비어 있습니다 (BossLobbyExperience 확인)"));
		return;
	}

	UCommonSession_HostSessionRequest* HostRequest = LobbyExperience->CreateHostingRequest(this);
	HostRequest->OnlineMode = Actor->GetSessionOnlineMode();
	// LAN(NULL 서브시스템)은 로비 기능이 없으므로 일반 게임 세션으로 만든다
	HostRequest->bUseLobbies = (HostRequest->OnlineMode == ECommonSessionOnlineMode::Online);

	FString RoomName = RoomNameInput->GetText().ToString().TrimStartAndEnd();
	if (RoomName.IsEmpty())
	{
		const APlayerState* PlayerState = GetOwningPlayerState();
		RoomName = FString::Printf(TEXT("%s의 원정"), PlayerState ? *PlayerState->GetPlayerName() : TEXT("플레이어"));
	}
	HostRequest->SessionStringSettings.Add(NAME_LeeBossRoomName, RoomName);
	HostRequest->SessionStringSettings.Add(NAME_LeeBossBossName, Actor->GetBossName().ToString());

	// 26.11.02 추가 - 로비/보스맵에서 돌아올 필드맵 기억
	if (ULeeBossExpeditionSubsystem* ExpeditionSubsystem = ULeeBossExpeditionSubsystem::Get(this))
	{
		ExpeditionSubsystem->BeginExpedition(GetWorld());
	}

	SetStatusText(NSLOCTEXT("LeeBossLobby", "Creating", "방을 만드는 중..."));
	SessionSubsystem->HostSession(GetOwningPlayer(), HostRequest);
}

void ULeeBossLobbyWidget::HandleRefreshClicked()
{
	RefreshRoomList();
}

// 창 닫기
void ULeeBossLobbyWidget::HandleCloseClicked()
{
	DeactivateWidget();
}

// 같은 세션 방식으로 검색 요청을 보낸다 (완료 시 HandleSearchFinished)
void ULeeBossLobbyWidget::RefreshRoomList()
{
	const ALeeBossRequestActor* Actor = RequestActor.Get();
	UCommonSessionSubsystem* SessionSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCommonSessionSubsystem>() : nullptr;
	// 이전 검색이 끝나기 전에는 새 검색을 보내지 않는다 (겹치면 OSS가 새 요청을 거부함)
	if (!Actor || !SessionSubsystem || bIsSearching)
	{
		return;
	}

	bIsSearching = true;
	RefreshButton->SetIsEnabled(false);

	CurrentSearchRequest = SessionSubsystem->CreateOnlineSearchSessionRequest();
	CurrentSearchRequest->OnlineMode = Actor->GetSessionOnlineMode();
	CurrentSearchRequest->bUseLobbies = (CurrentSearchRequest->OnlineMode == ECommonSessionOnlineMode::Online);
	CurrentSearchRequest->OnSearchFinished.AddUObject(this, &ThisClass::HandleSearchFinished);

	RoomListPanel->ClearChildren();
	SetStatusText(NSLOCTEXT("LeeBossLobby", "Searching", "방을 찾는 중..."));
	SessionSubsystem->FindSessions(GetOwningPlayer(), CurrentSearchRequest);
}

// 방 이름 설정이 있는 세션(= 우리 보스 방)만 항목으로 만든다
void ULeeBossLobbyWidget::HandleSearchFinished(bool bSucceeded, const FText& ErrorMessage)
{
	bIsSearching = false;
	RefreshButton->SetIsEnabled(true);

	RoomListPanel->ClearChildren();

	if (!bSucceeded || !CurrentSearchRequest)
	{
		SetStatusText(ErrorMessage.IsEmpty() ? NSLOCTEXT("LeeBossLobby", "SearchFailed", "방 검색에 실패했습니다") : ErrorMessage);
		return;
	}

	int32 RoomCount = 0;
	for (UCommonSession_SearchResult* Result : CurrentSearchRequest->Results)
	{
		FString RoomName;
		FString BossName;
		bool bFoundRoomName = false;
		bool bFoundBossName = false;
		Result->GetStringSetting(NAME_LeeBossRoomName, RoomName, bFoundRoomName);
		Result->GetStringSetting(NAME_LeeBossBossName, BossName, bFoundBossName);
		if (!bFoundRoomName || !RoomEntryClass)
		{
			continue;
		}

		const int32 MaxPlayers = Result->GetMaxPublicConnections();
		const int32 CurrentPlayers = MaxPlayers - Result->GetNumOpenPublicConnections();

		ULeeBossRoomEntryWidget* Entry = CreateWidget<ULeeBossRoomEntryWidget>(this, RoomEntryClass);
		Entry->SetRoomInfo(FText::FromString(RoomName), FText::FromString(BossName), CurrentPlayers, MaxPlayers, Result, this);
		RoomListPanel->AddChild(Entry);
		++RoomCount;
	}

	SetStatusText(RoomCount > 0
		? FText::Format(NSLOCTEXT("LeeBossLobby", "RoomCount", "방 {0}개"), RoomCount)
		: NSLOCTEXT("LeeBossLobby", "NoRoom", "참여할 수 있는 방이 없습니다"));
}

// 26.11.02 변경 - 선택한 방의 호스트에 입장 요청 (수락되면 대기 패널이 세션 참가 → 로비 맵 이동)
void ULeeBossLobbyWidget::JoinRoom(UCommonSession_SearchResult* SearchResult)
{
	if (!SearchResult)
	{
		return;
	}

	SetStatusText(FText::GetEmpty());
	JoinWaitPanel->StartJoinRequest(SearchResult);
}

// 상태 문구 표시 (StatusText가 있을 때만)
void ULeeBossLobbyWidget::SetStatusText(const FText& InText)
{
	if (StatusText)
	{
		StatusText->SetText(InText);
	}
}
