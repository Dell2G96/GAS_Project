#include "PIEAutoRecorderSettings.h"

#include "PIEAutoRecorderLog.h"
#include "HAL/PlatformMisc.h"

namespace
{
	// 비밀번호를 담는 환경변수 이름. INI보다 우선한다.
	const TCHAR* PasswordEnvVarName = TEXT("OBS_WEBSOCKET_PASSWORD");
}

// 설정 화면에서 보일 섹션 이름을 지정한다.
UPIEAutoRecorderSettings::UPIEAutoRecorderSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("PIE Auto Recorder");
}

// Project Settings의 'Plugins' 분류 아래에 표시되도록 한다.
FName UPIEAutoRecorderSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

// 환경변수 → 개인 INI 순서로 비밀번호를 읽는다. 값 자체는 절대 로그로 남기지 않는다.
FString UPIEAutoRecorderSettings::ResolvePassword() const
{
	const FString FromEnv = FPlatformMisc::GetEnvironmentVariable(PasswordEnvVarName);
	if (!FromEnv.IsEmpty())
	{
		return FromEnv;
	}

	return Password;
}

// 현재 설정을 한눈에 볼 수 있게 로그로 남긴다. 비밀번호는 설정 여부만 출력한다.
void UPIEAutoRecorderSettings::LogCurrentSettings() const
{
	UE_LOG(LogPIEAutoRecorder, Log, TEXT("설정: AutoRecording=%s, Server=%s:%d, Password=%s"),
		bEnableAutoRecording ? TEXT("켜짐") : TEXT("꺼짐"),
		*ServerHost,
		ServerPort,
		ResolvePassword().IsEmpty() ? TEXT("(없음)") : TEXT("(설정됨)"));

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("설정: ConnectWhenEditorStarts=%s, PromptOnPIEEnd=%s, DiscardBehavior=%s"),
		bConnectWhenEditorStarts ? TEXT("켜짐") : TEXT("꺼짐"),
		bPromptOnPIEEnd ? TEXT("켜짐") : TEXT("꺼짐"),
		DiscardBehavior == EPIERecordingDiscardBehavior::DeleteFile ? TEXT("파일 삭제") : TEXT("원본 유지"));
}
