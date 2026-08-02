#pragma once

#include "CoreMinimal.h"

// 에디터 알림(Notification)을 한 곳에서 처리한다.
// 모달 오류 팝업은 절대 쓰지 않는다. 알림과 로그만 사용한다(계획서 §2.3-2).
namespace PIEAutoRecorderNotification
{
	// 성공 알림. bShowSuccessNotification 설정이 꺼져 있으면 표시하지 않는다.
	void ShowSuccess(const FString& Message);

	// 실패·경고 알림. bShowFailureNotification 설정이 꺼져 있으면 표시하지 않는다.
	void ShowFailure(const FString& Message);
}
