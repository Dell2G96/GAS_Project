#pragma once

#include "CoreMinimal.h"
#include "IOBSProcessPlatform.h"

// IOBSProcessPlatform의 Windows 구현.
// 프로세스 이름만으로 대상을 고르지 않고 실행 경로 + PID + CreationTime을 함께 대조한다.
class FWindowsOBSProcessPlatform : public IOBSProcessPlatform
{
public:
	virtual FOBSProcessInfo FindRunningOBS(const FString& ExecutablePath) override;
	virtual FOBSProcessInfo LaunchOBS(const FString& ExecutablePath, const FString& Arguments, const FString& WorkingDirectory) override;
	virtual bool IsProcessRunning(const FOBSProcessInfo& ProcessInfo) override;
	virtual bool RequestGracefulClose(const FOBSProcessInfo& ProcessInfo) override;
	virtual void Release(FOBSProcessInfo& ProcessInfo) override;
};
