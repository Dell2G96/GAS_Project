#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "PIEAutoRecorderSettings.generated.h"

// 저장 확인 창에서 '저장 안 함'을 눌렀을 때의 동작.
UENUM()
enum class EPIERecordingDiscardBehavior : uint8
{
	// 녹화 파일을 삭제한다. (되돌릴 수 없음)
	DeleteFile		UMETA(DisplayName = "파일 삭제"),

	// 파일을 지우지 않고 OBS 원본 경로에 그대로 둔다.
	KeepInPlace		UMETA(DisplayName = "원본 유지"),
};

// 아주 짧은 녹화(MinDurationToPromptSeconds 미만)를 어떻게 처리할지.
UENUM()
enum class EPIEShortRecordingBehavior : uint8
{
	// 평소처럼 저장 확인 창을 띄운다.
	Prompt			UMETA(DisplayName = "창 표시"),

	// 창 없이 기본 경로·기본 이름으로 저장한다.
	AutoSave		UMETA(DisplayName = "자동 저장"),

	// 창 없이 삭제한다.
	AutoDelete		UMETA(DisplayName = "자동 삭제"),
};

// PIE 자동 녹화 플러그인의 개인 설정.
// EditorPerProjectUserSettings에만 저장되므로 값이 프로젝트 공유 INI로 새지 않는다.
UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "PIE Auto Recorder"))
class UPIEAutoRecorderSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPIEAutoRecorderSettings();

	virtual FName GetCategoryName() const override;

	// 현재 설정 값을 로그로 남긴다. 비밀번호는 설정 여부만 출력한다.
	void LogCurrentSettings() const;

	// 환경변수 → 개인 INI 순서로 비밀번호를 읽는다. 둘 다 비면 빈 문자열.
	FString ResolvePassword() const;

	// 폴더 선택 설정을 우선 사용하고, 비어 있으면 이전 문자열 설정을 반환한다.
	FString ResolveDefaultSaveDirectory() const;

	//~ 기본 동작 ------------------------------------------------------------

	// 전체 on/off. 기본 off — 팀원이 모르는 사이에 녹화되지 않게 한다.
	UPROPERTY(config, EditAnywhere, Category = "동작")
	bool bEnableAutoRecording = false;

	// Simulate In Editor(SIE)도 녹화할지 여부.
	UPROPERTY(config, EditAnywhere, Category = "동작")
	bool bRecordSimulateInEditor = false;

	//~ 연결 ----------------------------------------------------------------

	// OBS WebSocket 호스트. 원격 주소는 권장하지 않는다.
	UPROPERTY(config, EditAnywhere, Category = "연결")
	FString ServerHost = TEXT("127.0.0.1");

	// obs-websocket 5.x 기본 포트.
	UPROPERTY(config, EditAnywhere, Category = "연결", meta = (ClampMin = "1", ClampMax = "65535"))
	int32 ServerPort = 4455;

	// 비워두면 인증 없이 연결한다. 환경변수 OBS_WEBSOCKET_PASSWORD가 있으면 그쪽이 우선이다.
	UPROPERTY(config, EditAnywhere, Category = "연결", meta = (PasswordField = true))
	FString Password;

	// 에디터 시작 시 미리 연결해 PIE 시작 지연을 줄인다.
	UPROPERTY(config, EditAnywhere, Category = "연결")
	bool bConnectWhenEditorStarts = true;

	// 1회 연결 제한 시간(초).
	UPROPERTY(config, EditAnywhere, Category = "연결", meta = (ClampMin = "0.1"))
	float ConnectTimeoutSeconds = 3.0f;

	// 요청 응답 제한 시간(초).
	UPROPERTY(config, EditAnywhere, Category = "연결", meta = (ClampMin = "0.1"))
	float RequestTimeoutSeconds = 2.0f;

	// 지수 백오프 재연결 최대 횟수.
	UPROPERTY(config, EditAnywhere, Category = "연결", meta = (ClampMin = "0"))
	int32 MaxReconnectAttempts = 3;

	//~ 녹화 ----------------------------------------------------------------

	// 정지 지연(초). 기본 0 — 지연은 빠른 재시작 경합의 원인이 된다.
	UPROPERTY(config, EditAnywhere, Category = "녹화", meta = (ClampMin = "0.0"))
	float StopDelaySeconds = 0.0f;

	//~ 알림 ----------------------------------------------------------------

	// 시작·완료 알림 표시.
	UPROPERTY(config, EditAnywhere, Category = "알림")
	bool bShowSuccessNotification = true;

	// 실패 알림 표시.
	UPROPERTY(config, EditAnywhere, Category = "알림")
	bool bShowFailureNotification = true;

	//~ 저장 확인 창 --------------------------------------------------------

	// PIE 종료 후 저장 확인 창을 띄운다.
	UPROPERTY(config, EditAnywhere, Category = "저장 확인 창")
	bool bPromptOnPIEEnd = true;

	// 창을 모달로 띄운다. 켜면 연속 PIE가 막히므로 권장하지 않는다.
	UPROPERTY(config, EditAnywhere, Category = "저장 확인 창")
	bool bModalPrompt = false;

	// 창과 자동 저장에서 사용할 기본 폴더. 비우면 OBS 원본 폴더를 그대로 쓴다.
	UPROPERTY(config, EditAnywhere, Category = "저장 확인 창", meta = (DisplayName = "기본 저장 폴더"))
	FDirectoryPath DefaultSaveDirectoryPath;

	// 이전 버전의 문자열 설정값. 기존 개인 INI 호환을 위해 읽기만 한다.
	UPROPERTY(config)
	FString DefaultSaveDirectory;

	// 창의 기본 파일명 형식. {level} {date} {time} {id} 치환.
	UPROPERTY(config, EditAnywhere, Category = "저장 확인 창")
	FString SaveFilenameFormat = TEXT("PIE_{level}_{date}_{time}");

	// '저장 안 함'을 눌렀을 때의 동작.
	UPROPERTY(config, EditAnywhere, Category = "저장 확인 창")
	EPIERecordingDiscardBehavior DiscardBehavior = EPIERecordingDiscardBehavior::DeleteFile;

	// 창 없이 항상 기본 경로·기본 이름으로 저장한다.
	UPROPERTY(config, EditAnywhere, Category = "저장 확인 창")
	bool bAutoSaveWithoutPrompt = false;

	// 이보다 짧은 녹화는 창 없이 ShortRecordingBehavior대로 처리한다. 0이면 항상 창을 띄운다.
	UPROPERTY(config, EditAnywhere, Category = "저장 확인 창", meta = (ClampMin = "0.0"))
	float MinDurationToPromptSeconds = 0.0f;

	// 짧은 녹화의 처리 방식.
	UPROPERTY(config, EditAnywhere, Category = "저장 확인 창")
	EPIEShortRecordingBehavior ShortRecordingBehavior = EPIEShortRecordingBehavior::Prompt;

	// 파일 잠금(리먹싱) 해제 대기 제한 시간(초).
	UPROPERTY(config, EditAnywhere, Category = "저장 확인 창", meta = (ClampMin = "0.0"))
	float FileReadyTimeoutSeconds = 5.0f;

	//~ OBS 프로그램 ----------------------------------------------------------
	// 이 카테고리는 OBS "프로그램 실행"만 다룬다. 위쪽 "연결" 카테고리의 WebSocket 접속 설정과는 별개다.

	// OBS 실행 파일 경로. 비워두면 실행하지 않고 안내만 한다.
	UPROPERTY(config, EditAnywhere, Category = "OBS 프로그램")
	FFilePath OBSExecutablePath;

	// 최소화(트레이) 상태로 실행할지 여부. --minimize-to-tray 인자로 반영된다.
	UPROPERTY(config, EditAnywhere, Category = "OBS 프로그램")
	bool bLaunchOBSMinimized = true;

	// 실행 성공 판정 제한 시간(초).
	UPROPERTY(config, EditAnywhere, Category = "OBS 프로그램", meta = (ClampMin = "1.0"))
	float OBSStartupTimeoutSeconds = 10.0f;

	// 정상 종료 제한 시간(초). 넘기면 강제 종료하지 않고 경고만 남긴다.
	UPROPERTY(config, EditAnywhere, Category = "OBS 프로그램", meta = (ClampMin = "1.0"))
	float OBSShutdownTimeoutSeconds = 10.0f;

	// Editor 종료 시 플러그인 소유 OBS를 함께 정상 종료할지 여부.
	UPROPERTY(config, EditAnywhere, Category = "OBS 프로그램")
	bool bCloseOwnedOBSOnEditorExit = true;

	// 사용자 추가 커맨드라인 인자. 기본 인자(--disable-updater 등) 뒤에 그대로 덧붙는다.
	// 비밀번호(--websocket_password)는 여기 넣지 않는다 — 프로세스 목록에 노출되므로 절대 금지.
	UPROPERTY(config, EditAnywhere, Category = "OBS 프로그램")
	FString OBSLaunchArguments;
};
