// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeBossRoomEntryWidget.h"

#include "LeeBossLobbyWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

// '입장' 버튼 클릭 바인딩
void ULeeBossRoomEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	JoinButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleJoinClicked);
}

// 방 정보를 텍스트에 채우고, 가득 찬 방은 '입장' 버튼을 비활성화
void ULeeBossRoomEntryWidget::SetRoomInfo(const FText& RoomName, const FText& BossName, int32 CurrentPlayers, int32 MaxPlayers,
	UCommonSession_SearchResult* InSearchResult, ULeeBossLobbyWidget* InOwnerLobby)
{
	SearchResult = InSearchResult;
	OwnerLobby = InOwnerLobby;

	RoomNameText->SetText(RoomName);
	BossNameText->SetText(BossName);
	PlayerCountText->SetText(FText::Format(NSLOCTEXT("LeeBossLobby", "PlayerCount", "{0}/{1}"), CurrentPlayers, MaxPlayers));
	JoinButton->SetIsEnabled(CurrentPlayers < MaxPlayers);
}

// 로비 창에 입장을 요청
void ULeeBossRoomEntryWidget::HandleJoinClicked()
{
	if (ULeeBossLobbyWidget* Lobby = OwnerLobby.Get())
	{
		Lobby->JoinRoom(SearchResult);
	}
}
