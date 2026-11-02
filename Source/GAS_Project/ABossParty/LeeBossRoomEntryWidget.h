// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "LeeBossRoomEntryWidget.generated.h"

class UButton;
class UCommonSession_SearchResult;
class ULeeBossLobbyWidget;
class UTextBlock;

// 26.11.02 추가 - 로비 방 목록의 방 1개. 방 이름 / 보스 이름 / 현재 인원·최대 인원과 '입장' 버튼을 표시한다
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeBossRoomEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 표시할 방 정보와 입장 시 사용할 검색 결과를 설정
	void SetRoomInfo(const FText& RoomName, const FText& BossName, int32 CurrentPlayers, int32 MaxPlayers,
		UCommonSession_SearchResult* InSearchResult, ULeeBossLobbyWidget* InOwnerLobby);

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void HandleJoinClicked();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RoomNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText;

	// "현재/최대" 형식 (예: 1/3)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> JoinButton;

private:
	UPROPERTY()
	TObjectPtr<UCommonSession_SearchResult> SearchResult;

	TWeakObjectPtr<ULeeBossLobbyWidget> OwnerLobby;
};
