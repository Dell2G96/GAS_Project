#pragma once

#include "CoreMinimal.h"

// OBS 프로세스 하나를 식별하는 데 필요한 최소 정보.
// PID만으로는 재사용 위험이 있어 CreationTime까지 함께 대조한다.
struct FOBSProcessInfo
{
	FProcHandle Handle;
	uint32      ProcessId = 0;
	FString     ExecutablePath;
	bool        bOwned = false;
	FDateTime   CreationTime;

	bool IsValid() const
	{
		return ProcessId != 0;
	}
};

// OS 프로세스 제어를 Controller에서 분리하는 인터페이스.
// Automation Test에서는 Fake 구현으로 실제 OBS 실행 없이 상태 전이를 검증한다.
class IOBSProcessPlatform
{
public:
	virtual ~IOBSProcessPlatform() = default;

	// 지정한 실행 파일 경로와 일치하는 실행 중 OBS를 찾는다. 없으면 IsValid()가 false인 값을 돌려준다.
	virtual FOBSProcessInfo FindRunningOBS(const FString& ExecutablePath) = 0;

	// OBS를 새로 실행한다. 실패하면 IsValid()가 false인 값을 돌려준다.
	virtual FOBSProcessInfo LaunchOBS(const FString& ExecutablePath, const FString& Arguments, const FString& WorkingDirectory) = 0;

	// Handle이 가리키는 프로세스가 아직 살아 있는지 확인한다.
	virtual bool IsProcessRunning(const FOBSProcessInfo& ProcessInfo) = 0;

	// 소유 PID와 일치하는 Top-Level Window에 정상 종료 메시지(WM_CLOSE)를 보낸다.
	// 강제 종료(TerminateProc)는 이 함수의 책임이 아니다.
	virtual bool RequestGracefulClose(const FOBSProcessInfo& ProcessInfo) = 0;

	// Handle을 해제한다. 프로세스 자체를 죽이지 않는다.
	virtual void Release(FOBSProcessInfo& ProcessInfo) = 0;
};
