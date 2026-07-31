#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FOBSWebSocketBackend;
struct IConsoleCommand;
struct FOBSRecordStateChanged;

// 플러그인 진입점. 수명 관리와 PIE 델리게이트 등록/해제만 담당하고 판단 로직은 갖지 않는다.
// 3단계에서는 백엔드를 소유하고, 수동 검증용 콘솔 명령을 등록한다.
class FPIEAutoRecorderModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	// 3단계 검증용 콘솔 명령을 등록한다. Coordinator가 붙는 4단계까지의 임시 수단이다.
	void RegisterConsoleCommands();
	void UnregisterConsoleCommands();

	// 녹화 상태 변화 이벤트를 로그로 남긴다.
	void HandleRecordStateChanged(const FOBSRecordStateChanged& Event);

	// 콘솔 명령을 실행할 수 있는 상태인지 확인하고, 아니면 해결 방법을 로그로 안내한다.
	// 사용자가 직접 명령을 쳤는데 아무 반응이 없는 상황을 막기 위한 것이다.
	bool CanRunConsoleCommand() const;

	TSharedPtr<FOBSWebSocketBackend> Backend;

	TArray<IConsoleCommand*> ConsoleCommands;
};
