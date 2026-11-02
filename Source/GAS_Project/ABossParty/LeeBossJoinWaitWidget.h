// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "LeeBossJoinBeaconClient.h"
#include "LeeBossJoinWaitWidget.generated.h"

class UButton;
class UCommonSession_SearchResult;
class UTextBlock;

// 26.11.02 추가 - 방 [입장] 후 "요청 수락을 기다리는 중 . . ." 패널 (로비 창 안에 겹쳐 배치, 평소에는 숨김)
// 입장 요청 비콘으로 호스트에 요청을 보내고, 수락되면 세션 참가 / 거절·실패면 안내 문구 + [확인] / [취소]로 대기 종료
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeBossJoinWaitWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 패널을 띄우고 선택한 방의 호스트에 입장 요청 전송
	void StartJoinRequest(UCommonSession_SearchResult* InSearchResult);

	// 대기 중인 요청을 끊고 패널 숨김
	void HideWaitPanel();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleCancelClicked();

	UFUNCTION()
	void HandleConfirmClicked();

	// 비콘 결과 처리 — 수락: 세션 참가 / 거절·실패: 안내 문구
	void HandleJoinResult(ELeeBossJoinResult Result);

	// 안내 문구 + [확인] 버튼만 표시
	void ShowResultMessage(const FText& Message);

	// 진행 중인 요청 비콘 정리
	void ReleaseBeacon();

protected:
	// "요청 수락을 기다리는 중 . . ." / 거절·실패 안내 문구
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WaitText;

	// 대기 중일 때만 표시
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CancelButton;

	// 거절·실패 안내 후에만 표시
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConfirmButton;

private:
	// 수락 시 참가할 세션 (GC 방지)
	UPROPERTY()
	TObjectPtr<UCommonSession_SearchResult> SearchResult;

	TWeakObjectPtr<ALeeBossJoinBeaconClient> BeaconClient;
};
