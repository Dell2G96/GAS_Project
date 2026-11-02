// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeBossJoinWaitWidget.h"

#include "CommonSessionSubsystem.h"
#include "LeeBossExpeditionSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineSessionInterface.h"

// 버튼 클릭 바인딩
void ULeeBossJoinWaitWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	CancelButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCancelClicked);
	ConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirmClicked);
}

// 위젯이 사라지면 요청도 끊는다 (호스트 목록에서 자동 제거됨)
void ULeeBossJoinWaitWidget::NativeDestruct()
{
	ReleaseBeacon();

	Super::NativeDestruct();
}

// 검색 결과에서 호스트의 비콘 주소를 얻어 입장 요청 비콘을 접속시킨다
void ULeeBossJoinWaitWidget::StartJoinRequest(UCommonSession_SearchResult* InSearchResult)
{
	ReleaseBeacon();
	SearchResult = InSearchResult;

	SetVisibility(ESlateVisibility::Visible);
	WaitText->SetText(NSLOCTEXT("LeeBossLobby", "WaitApproval", "요청 수락을 기다리는 중 . . ."));
	CancelButton->SetVisibility(ESlateVisibility::Visible);
	ConfirmButton->SetVisibility(ESlateVisibility::Collapsed);

	FString ConnectInfo;
#if COMMONUSER_OSSV1
	const IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
	if (!SearchResult || !SessionInterface.IsValid()
		|| !SessionInterface->GetResolvedConnectString(SearchResult->Result, NAME_BeaconPort, ConnectInfo))
	{
		ShowResultMessage(NSLOCTEXT("LeeBossLobby", "NoConnectInfo", "방에 연결할 수 없습니다"));
		return;
	}
#endif

	ALeeBossJoinBeaconClient* NewBeacon = GetWorld()->SpawnActor<ALeeBossJoinBeaconClient>();
	const APlayerState* PlayerState = GetOwningPlayerState();
	const FString PlayerName = PlayerState ? PlayerState->GetPlayerName() : TEXT("플레이어");

	if (!NewBeacon)
	{
		ShowResultMessage(NSLOCTEXT("LeeBossLobby", "NoConnectInfo", "방에 연결할 수 없습니다"));
		return;
	}

	BeaconClient = NewBeacon;
	NewBeacon->OnJoinResult.BindUObject(this, &ThisClass::HandleJoinResult);
	if (!NewBeacon->RequestJoin(ConnectInfo, PlayerName))
	{
		ReleaseBeacon();
		ShowResultMessage(NSLOCTEXT("LeeBossLobby", "NoConnectInfo", "방에 연결할 수 없습니다"));
	}
}

// 요청 비콘 정리 후 패널 숨김
void ULeeBossJoinWaitWidget::HideWaitPanel()
{
	ReleaseBeacon();
	SetVisibility(ESlateVisibility::Collapsed);
}

// [취소] → 대기 종료 (호스트 쪽 요청도 연결이 끊기며 사라진다)
void ULeeBossJoinWaitWidget::HandleCancelClicked()
{
	HideWaitPanel();
}

// [확인] → 안내 문구 닫고 방 목록으로
void ULeeBossJoinWaitWidget::HandleConfirmClicked()
{
	HideWaitPanel();
}

// 수락이면 원정 시작 표시 후 세션 참가 (기존 예약 비콘 → 로비 맵 이동), 거절/실패면 안내
void ULeeBossJoinWaitWidget::HandleJoinResult(ELeeBossJoinResult Result)
{
	// 비콘은 결과를 보낸 뒤 스스로 정리되므로 참조만 놓는다
	BeaconClient.Reset();

	switch (Result)
	{
	case ELeeBossJoinResult::Approved:
		{
			WaitText->SetText(NSLOCTEXT("LeeBossLobby", "Approved", "수락되었습니다. 입장하는 중..."));
			CancelButton->SetVisibility(ESlateVisibility::Collapsed);
			ConfirmButton->SetVisibility(ESlateVisibility::Collapsed);

			if (ULeeBossExpeditionSubsystem* ExpeditionSubsystem = ULeeBossExpeditionSubsystem::Get(this))
			{
				ExpeditionSubsystem->BeginExpedition(GetWorld());
			}
			if (UCommonSessionSubsystem* SessionSubsystem = GetGameInstance()->GetSubsystem<UCommonSessionSubsystem>())
			{
				SessionSubsystem->JoinSession(GetOwningPlayer(), SearchResult);
			}
			break;
		}
	case ELeeBossJoinResult::Rejected:
		ShowResultMessage(NSLOCTEXT("LeeBossLobby", "Rejected", "호스트가 요청을 거절했습니다"));
		break;
	default:
		ShowResultMessage(NSLOCTEXT("LeeBossLobby", "RoomGone", "방이 사라졌습니다"));
		break;
	}
}

// 안내 문구만 남기고 [확인] 버튼 표시
void ULeeBossJoinWaitWidget::ShowResultMessage(const FText& Message)
{
	WaitText->SetText(Message);
	CancelButton->SetVisibility(ESlateVisibility::Collapsed);
	ConfirmButton->SetVisibility(ESlateVisibility::Visible);
}

// 결과 콜백을 끊고 비콘 파괴 (호스트에는 연결 끊김으로 전달되어 요청 목록에서 빠진다)
void ULeeBossJoinWaitWidget::ReleaseBeacon()
{
	if (ALeeBossJoinBeaconClient* Beacon = BeaconClient.Get())
	{
		Beacon->OnJoinResult.Unbind();
		Beacon->DestroyBeacon();
	}
	BeaconClient.Reset();
}
