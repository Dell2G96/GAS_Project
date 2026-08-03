#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Types/SlateEnums.h"
#include "OBS/OBSProcessController.h"

class FOBSWebSocketBackend;
class FPIERecordingCoordinator;
class FRecordingDispositionQueue;
class IOBSProcessPlatform;
class SWidget;
struct IConsoleCommand;
struct FOBSRecordStateChanged;
enum class EOBSShutdownBlockReason : uint8;

// 플러그인 진입점. 수명 관리와 PIE 델리게이트 등록/해제만 담당하고 판단 로직은 갖지 않는다.
// OBS 프로그램 실행 체크박스의 오케스트레이션(실행 요청, 종료 준비, WebSocket 연결 연계)도 여기서 담당한다.
class FPIEAutoRecorderModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	// 3단계 검증용 콘솔 명령을 등록한다.
	void RegisterConsoleCommands();
	void UnregisterConsoleCommands();

	// 녹화 상태 변화 이벤트를 로그로 남긴다.
	void HandleRecordStateChanged(const FOBSRecordStateChanged& Event);

	// 콘솔 명령을 실행할 수 있는 상태인지 확인하고, 아니면 해결 방법을 로그로 안내한다.
	bool CanRunConsoleCommand() const;

	// PIE 델리게이트를 Coordinator에 연결한다. 해제와 반드시 대칭이어야 한다.
	void RegisterPIEDelegates();
	void UnregisterPIEDelegates();

	// 설정이 바뀌면 연결에 영향을 주는 항목인지 보고 안전할 때만 다시 연결한다.
	void HandleSettingsChanged(UObject* Object, struct FPropertyChangedEvent& PropertyChangedEvent);

	//~ 툴바 OBS 체크박스 ------------------------------------------------------

	// LevelEditor.LevelEditorToolBar.User 슬롯에 체크박스를 등록한다. 대칭적으로 해제해야 한다.
	void RegisterToolbar();
	void UnregisterToolbar();

	TSharedRef<SWidget> CreateOBSToggleWidget();

	ECheckBoxState GetOBSCheckState() const;
	FText GetOBSLabel() const;
	FText GetOBSToolTip() const;
	bool GetOBSToggleEnabled() const;
	void HandleOBSToggleChanged(ECheckBoxState NewState);

	// Controller 상태가 바뀔 때마다 호출된다. WebSocket 연결/해제를 여기서 연계한다.
	void HandleOBSProcessStateChanged(EOBSProcessState NewState);

	// 체크 해제 흐름. 소유 프로세스면 Coordinator에 종료 준비를 요청한다.
	void BeginOwnedOBSShutdown();
	void OnRecordingReadyForOBSShutdown();
	void OnRecordingRejectedForOBSShutdown(EOBSShutdownBlockReason Reason, FString Message);

	// 에디터 종료 시 소유 OBS를 정리한다(§14.2). Coordinator의 PIE 정리 이후에 호출된다.
	void HandleEditorPreExitForOBS();

	TSharedPtr<FOBSWebSocketBackend>       Backend;
	TSharedPtr<FRecordingDispositionQueue> DispositionQueue;
	TUniquePtr<FPIERecordingCoordinator>   Coordinator;

	TSharedPtr<IOBSProcessPlatform>   ProcessPlatform;
	TUniquePtr<FOBSProcessController> ProcessController;
	FDelegateHandle                   ProcessStateChangedHandle;
	FDelegateHandle                   ToolbarStartupHandle;

	// Coordinator 콜백(OnReady/OnRejected)이 파괴된 Module을 참조하지 못하도록 막는 패턴(§13.3).
	// Module은 UObject가 아니므로 TWeakObjectPtr를 쓸 수 없어 bool 플래그로 생존 여부를 표시한다.
	TSharedPtr<bool> bIsAlive;

	TArray<IConsoleCommand*> ConsoleCommands;

	FDelegateHandle PostPIEStartedHandle;
	FDelegateHandle PrePIEEndedHandle;
	FDelegateHandle ShutdownPIEHandle;
	FDelegateHandle CancelPIEHandle;
	FDelegateHandle EditorPreExitHandle;
	FDelegateHandle SettingsChangedHandle;
};
