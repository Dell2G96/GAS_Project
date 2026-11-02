// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeBossJoinRequestEntryWidget.h"

#include "LeeBossJoinBeaconClient.h"
#include "LeeBossJoinBeaconHostObject.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

// 버튼 클릭 바인딩
void ULeeBossJoinRequestEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	AcceptButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleAcceptClicked);
	RejectButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRejectClicked);
}

// 요청자 이름 표시 + 수락/거절 대상 저장
void ULeeBossJoinRequestEntryWidget::SetRequest(ALeeBossJoinBeaconClient* InRequest, ALeeBossJoinBeaconHostObject* InHostObject)
{
	Request = InRequest;
	HostObject = InHostObject;

	PlayerNameText->SetText(FText::FromString(InRequest ? InRequest->GetRequestPlayerName() : FString()));
}

// [수락] → 요청자가 세션에 참가한다 (목록은 HostObject 알림으로 다시 그려짐)
void ULeeBossJoinRequestEntryWidget::HandleAcceptClicked()
{
	if (ALeeBossJoinBeaconHostObject* Host = HostObject.Get())
	{
		Host->ApproveRequest(Request.Get());
	}
}

// [거절] → 요청자에게 거절 안내
void ULeeBossJoinRequestEntryWidget::HandleRejectClicked()
{
	if (ALeeBossJoinBeaconHostObject* Host = HostObject.Get())
	{
		Host->RejectRequest(Request.Get());
	}
}
