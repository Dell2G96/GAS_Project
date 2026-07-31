#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// PIE 자동 녹화 플러그인 전용 로그 카테고리.
// 비밀번호·인증 문자열은 어떤 경우에도 이 카테고리로 출력하지 않는다.
DECLARE_LOG_CATEGORY_EXTERN(LogPIEAutoRecorder, Log, All);
