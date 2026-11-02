// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/AUI/LeeActivatableWidget.h"
#include "LeeBossLobbyHUDWidget.generated.h"

class ULeeBossJoinRequestEntryWidget;
class UButton;
class UPanelWidget;
class UTextBlock;
class UWidget;

// 26.11.02 추가 - 보스 로비 맵 HUD. 참가자 목록(준비 상태) / [준비] / 출발 카운트다운 / [나가기]
// 호스트에게만 입장 요청 목록(수락/거절)을 표시한다. ALeeBossLobbyManager가 로컬 플레이어 화면에 띄운다
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeBossLobbyHUDWidget : public ULeeActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	UFUNCTION()
	void HandleReadyClicked();

	UFUNCTION()
	void HandleLeaveClicked();

	// 참가자 목록 / 준비 버튼 문구 / 카운트다운을 주기적으로 갱신
	void RefreshLobbyState();

	// [호스트] 입장 요청 목록 다시 그리기 (HostObject 알림 시)
	void RebuildRequestList();

	// 이 화면의 플레이어가 호스트(리슨 서버)인지
	bool IsHost() const;

protected:
	// 참가자 이름 + 준비 상태 줄을 담는 패널 (VerticalBox 권장)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PlayerListPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ReadyButton;

	// "준비" / "준비 취소"
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ReadyButtonText;

	// "N초 후 출발" / "모든 플레이어가 준비하면 출발합니다"
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountdownText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LeaveButton;

	// 입장 요청 영역 전체 (클라이언트에게는 숨김)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> RequestPanel;

	// 입장 요청 항목을 담는 패널 (RequestPanel 안의 VerticalBox / ScrollBox)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> RequestListPanel;

	// 입장 요청 1건 항목 위젯 클래스 (ULeeBossJoinRequestEntryWidget 기반 BP)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|BossLobby")
	TSubclassOf<ULeeBossJoinRequestEntryWidget> RequestEntryClass;

	// 26.11.03 추가 - 참가자 줄 글자 크기
	UPROPERTY(EditDefaultsOnly, Category = "Lee|BossLobby|Style", meta = (ClampMin = 8))
	int32 PlayerLineFontSize = 16;

	// 26.11.03 추가 - 참가자 줄 글자 색 (대기 중)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|BossLobby|Style")
	FLinearColor PlayerLineColor = FLinearColor(0.745f, 0.708f, 0.604f);

	// 26.11.03 추가 - 참가자 줄 글자 색 (준비 완료)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|BossLobby|Style")
	FLinearColor ReadyLineColor = FLinearColor(0.694f, 0.479f, 0.171f);

	// 상태 갱신 주기(초)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|BossLobby", meta = (ClampMin = 0.05))
	float RefreshInterval = 0.25f;

private:
	FTimerHandle RefreshTimerHandle;

	FDelegateHandle RequestsChangedHandle;
};
