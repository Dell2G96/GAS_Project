// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "LeeBossJoinRequestEntryWidget.generated.h"

class ALeeBossJoinBeaconClient;
class ALeeBossJoinBeaconHostObject;
class UButton;
class UTextBlock;

// 26.11.02 추가 - [호스트] 로비 HUD의 입장 요청 1건. 요청자 이름과 [수락] / [거절] 버튼
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeBossJoinRequestEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// 표시할 요청과 결과를 보낼 HostObject 설정
	void SetRequest(ALeeBossJoinBeaconClient* InRequest, ALeeBossJoinBeaconHostObject* InHostObject);

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void HandleAcceptClicked();

	UFUNCTION()
	void HandleRejectClicked();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AcceptButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RejectButton;

private:
	TWeakObjectPtr<ALeeBossJoinBeaconClient> Request;

	TWeakObjectPtr<ALeeBossJoinBeaconHostObject> HostObject;
};
