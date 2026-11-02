// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeBossLobbyHUDWidget.h"

#include "LeeBossExpeditionSubsystem.h"
#include "LeeBossJoinBeaconHostObject.h"
#include "LeeBossJoinRequestEntryWidget.h"
#include "LeeBossLobbyManager.h"
#include "TimerManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GAS_Project/APlayer/LeePlayerController.h"

// 버튼 클릭 바인딩
void ULeeBossLobbyHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ReadyButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleReadyClicked);
	LeaveButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleLeaveClicked);
}

// 갱신 타이머 시작, 호스트면 입장 요청 목록 구독
void ULeeBossLobbyHUDWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	GetWorld()->GetTimerManager().SetTimer(RefreshTimerHandle, this, &ThisClass::RefreshLobbyState, RefreshInterval, true);
	RefreshLobbyState();

	RequestPanel->SetVisibility(IsHost() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	const ALeeBossLobbyManager* LobbyManager = ALeeBossLobbyManager::Get(this);
	if (ALeeBossJoinBeaconHostObject* HostObject = (IsHost() && LobbyManager) ? LobbyManager->GetJoinHostObject() : nullptr)
	{
		RequestsChangedHandle = HostObject->OnRequestsChanged.AddUObject(this, &ThisClass::RebuildRequestList);
	}
	RebuildRequestList();
}

// 타이머·구독 정리
void ULeeBossLobbyHUDWidget::NativeOnDeactivated()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}

	const ALeeBossLobbyManager* LobbyManager = ALeeBossLobbyManager::Get(this);
	if (ALeeBossJoinBeaconHostObject* HostObject = LobbyManager ? LobbyManager->GetJoinHostObject() : nullptr)
	{
		HostObject->OnRequestsChanged.Remove(RequestsChangedHandle);
	}

	Super::NativeOnDeactivated();
}

// [준비] ↔ [준비 취소] 토글 요청 (실제 상태는 서버의 로비 관리 Actor가 바꾼다)
void ULeeBossLobbyHUDWidget::HandleReadyClicked()
{
	const ALeeBossLobbyManager* LobbyManager = ALeeBossLobbyManager::Get(this);
	ALeePlayerController* PlayerController = Cast<ALeePlayerController>(GetOwningPlayer());
	if (LobbyManager && PlayerController)
	{
		PlayerController->ServerSetBossLobbyReady(!LobbyManager->IsPlayerReady(GetOwningPlayerState()));
	}
}

// [나가기] → 세션 정리 후 자기 필드맵으로 (호스트가 나가면 참가자도 연결이 끊겨 각자 필드로 돌아간다)
void ULeeBossLobbyHUDWidget::HandleLeaveClicked()
{
	if (ULeeBossExpeditionSubsystem* ExpeditionSubsystem = ULeeBossExpeditionSubsystem::Get(this))
	{
		ExpeditionSubsystem->ReturnToField();
	}
}

// GameState의 플레이어 목록으로 "이름 - 준비 완료/대기 중" 줄을 다시 만들고, 버튼 문구와 카운트다운 표시
void ULeeBossLobbyHUDWidget::RefreshLobbyState()
{
	const ALeeBossLobbyManager* LobbyManager = ALeeBossLobbyManager::Get(this);
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!LobbyManager || !GameState)
	{
		return;
	}

	PlayerListPanel->ClearChildren();
	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (!PlayerState || PlayerState->IsABot())
		{
			continue;
		}

		const bool bReady = LobbyManager->IsPlayerReady(PlayerState);

		UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Line->SetText(FText::Format(NSLOCTEXT("LeeBossLobby", "PlayerLine", "{0}  -  {1}"),
			FText::FromString(PlayerState->GetPlayerName()),
			bReady
				? NSLOCTEXT("LeeBossLobby", "Ready", "준비 완료")
				: NSLOCTEXT("LeeBossLobby", "NotReady", "대기 중")));

		// 26.11.03 추가 - 로비 위젯과 같은 스타일 (준비 완료 = 금색, 대기 중 = 양피지색, 그림자)
		FSlateFontInfo Font = Line->GetFont();
		Font.Size = PlayerLineFontSize;
		Line->SetFont(Font);
		Line->SetColorAndOpacity(FSlateColor(bReady ? ReadyLineColor : PlayerLineColor));
		Line->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Line->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));

		UPanelSlot* LineSlot = PlayerListPanel->AddChild(Line);
		if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(LineSlot))
		{
			VerticalSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}
	}

	// 26.11.03 추가 - 입장을 수락했지만 아직 접속 중인 인원 (이 인원이 도착할 때까지 출발하지 않는다)
	const int32 ArrivingCount = LobbyManager->GetArrivingPlayerCount();
	if (ArrivingCount > 0)
	{
		UTextBlock* ArrivingLine = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ArrivingLine->SetText(FText::Format(NSLOCTEXT("LeeBossLobby", "ArrivingLine", "접속 중...  ({0}명)"), ArrivingCount));

		FSlateFontInfo Font = ArrivingLine->GetFont();
		Font.Size = PlayerLineFontSize;
		ArrivingLine->SetFont(Font);
		ArrivingLine->SetColorAndOpacity(FSlateColor(PlayerLineColor));
		ArrivingLine->SetShadowOffset(FVector2D(1.0f, 1.0f));
		ArrivingLine->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));

		PlayerListPanel->AddChild(ArrivingLine);
	}

	ReadyButtonText->SetText(LobbyManager->IsPlayerReady(GetOwningPlayerState())
		? NSLOCTEXT("LeeBossLobby", "CancelReady", "준비 취소")
		: NSLOCTEXT("LeeBossLobby", "DoReady", "준비"));

	const float Remaining = LobbyManager->GetCountdownRemaining();
	CountdownText->SetText(Remaining >= 0.0f
		? FText::Format(NSLOCTEXT("LeeBossLobby", "Countdown", "{0}초 후 출발"), FMath::CeilToInt(Remaining))
		: NSLOCTEXT("LeeBossLobby", "WaitAllReady", "모든 플레이어가 준비하면 출발합니다"));
}

// [호스트] 대기 중인 요청마다 항목 위젯 생성
void ULeeBossLobbyHUDWidget::RebuildRequestList()
{
	RequestListPanel->ClearChildren();

	const ALeeBossLobbyManager* LobbyManager = ALeeBossLobbyManager::Get(this);
	ALeeBossJoinBeaconHostObject* HostObject = (IsHost() && LobbyManager) ? LobbyManager->GetJoinHostObject() : nullptr;
	if (!HostObject || !RequestEntryClass)
	{
		return;
	}

	for (ALeeBossJoinBeaconClient* Request : HostObject->GetPendingRequests())
	{
		ULeeBossJoinRequestEntryWidget* Entry = CreateWidget<ULeeBossJoinRequestEntryWidget>(this, RequestEntryClass);
		Entry->SetRequest(Request, HostObject);
		RequestListPanel->AddChild(Entry);
	}
}

// 리슨 서버의 로컬 플레이어만 호스트
bool ULeeBossLobbyHUDWidget::IsHost() const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	return PlayerController && PlayerController->HasAuthority();
}
