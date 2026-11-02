// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/AUI/LeeActivatableWidget.h"
#include "LeeBossLobbyWidget.generated.h"

class ALeeBossRequestActor;
class UButton;
class UCommonSession_SearchResult;
class UCommonSession_SearchSessionRequest;
struct FOnlineResultInformation;
class UEditableTextBox;
class ULeeBossJoinWaitWidget;
class ULeeBossRoomEntryWidget;
class UPanelWidget;
class UTextBlock;

// 26.11.02 추가 - 보스 원정 로비 창. 방 만들기(호스트) / 방 목록 검색 / 방 입장(클라이언트)을 담당한다
// 도움 요청 Actor(ALeeBossRequestActor)의 보스 이름·로비 Experience·세션 방식을 사용한다. 디자인은 BP에서 BindWidget으로 구성
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeBossLobbyWidget : public ULeeActivatableWidget
{
	GENERATED_BODY()

public:
	// 위젯을 띄운 도움 요청 Actor를 받아 저장하고 방 목록을 바로 검색한다
	void InitializeLobby(ALeeBossRequestActor* InRequestActor);

	// 방 항목의 '입장' 버튼에서 호출 — 26.11.02 변경: 바로 참가하지 않고 호스트에 입장 요청 (대기 패널 표시)
	void JoinRoom(UCommonSession_SearchResult* SearchResult);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	// 방 만들기 결과 — 실패하면 상태 문구에 이유 표시 (성공 시 로비 맵으로 이동하므로 처리 없음)
	void HandleCreateSessionComplete(const FOnlineResultInformation& Result);

	// 방 입장 결과 — 실패하면 상태 문구에 이유 표시
	void HandleJoinSessionComplete(const FOnlineResultInformation& Result);

	UFUNCTION()
	void HandleCreateRoomClicked();

	UFUNCTION()
	void HandleRefreshClicked();

	UFUNCTION()
	void HandleCloseClicked();

	// 세션 검색 시작 (완료 시 HandleSearchFinished)
	void RefreshRoomList();

	// 세션 검색 완료 → 우리 게임의 보스 방만 골라 목록 표시
	void HandleSearchFinished(bool bSucceeded, const FText& ErrorMessage);

	// 상태 문구 표시 (StatusText가 있을 때만)
	void SetStatusText(const FText& InText);

protected:
	// 호스트가 입력하는 방 이름 (비우면 "플레이어이름의 원정")
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> RoomNameInput;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CreateRoomButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	// 방 항목 위젯을 담는 패널 (ScrollBox / VerticalBox 등)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> RoomListPanel;

	// 검색 중 / 방 없음 / 입장 중 등의 안내 문구
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	// 26.11.02 추가 - "요청 수락을 기다리는 중" 패널 (로비 창 위에 겹쳐 배치, 기본 Collapsed)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULeeBossJoinWaitWidget> JoinWaitPanel;

	// 방 1개를 표시할 항목 위젯 클래스 (ULeeBossRoomEntryWidget 기반 BP)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|BossLobby")
	TSubclassOf<ULeeBossRoomEntryWidget> RoomEntryClass;

private:
	TWeakObjectPtr<ALeeBossRequestActor> RequestActor;

	// 진행 중인 검색 요청 (GC되지 않도록 보관)
	UPROPERTY()
	TObjectPtr<UCommonSession_SearchSessionRequest> CurrentSearchRequest;

	// 검색 진행 중 여부 (새로고침 연타로 검색이 겹치지 않게)
	bool bIsSearching = false;

	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
};
