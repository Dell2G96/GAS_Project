#include "PIEAutoRecorderNotification.h"

#include "PIEAutoRecorderSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

namespace PIEAutoRecorderNotification
{
	// 실제로 알림을 띄운다. Slate가 준비되지 않은 상황에서는 조용히 넘어간다.
	static void Show(const FString& Message, bool bSuccess)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return;
		}

		FNotificationInfo Info(FText::FromString(Message));

		// 실패 알림은 조금 더 오래 남긴다. 사용자가 조치를 취해야 하기 때문이다.
		Info.ExpireDuration = bSuccess ? 4.0f : 8.0f;
		Info.bFireAndForget = true;
		Info.bUseSuccessFailIcons = true;

		TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info);
		if (Item.IsValid())
		{
			Item->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		}
	}

	// 성공 알림. 설정으로 끌 수 있다.
	void ShowSuccess(const FString& Message)
	{
		const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
		if (Settings && !Settings->bShowSuccessNotification)
		{
			return;
		}

		Show(Message, true);
	}

	// 실패 알림. 설정으로 끌 수 있다.
	void ShowFailure(const FString& Message)
	{
		const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
		if (Settings && !Settings->bShowFailureNotification)
		{
			return;
		}

		Show(Message, false);
	}
}
