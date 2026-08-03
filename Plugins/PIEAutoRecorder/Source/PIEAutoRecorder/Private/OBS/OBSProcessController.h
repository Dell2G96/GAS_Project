#pragma once

#include "CoreMinimal.h"
#include "IOBSProcessPlatform.h"
#include "Containers/Ticker.h"

// OBS 프로그램 프로세스의 상태. §6.2 상태 전이 다이어그램에 명시된 전이만 허용한다.
enum class EOBSProcessState : uint8
{
	Stopped,
	Starting,
	RunningOwned,
	RunningExternal,
	PreparingShutdown,
	Closing,
	Failed,
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnOBSProcessStateChanged, EOBSProcessState /*NewState*/);

// OBS 프로그램 프로세스의 실행/종료/생존 감시를 담당하는 상태 머신.
// 녹화 상태는 전혀 알지 못한다. Module이 녹화 정리 완료를 알려준 뒤에만 종료를 수행한다.
// 모든 공개 API는 GameThread에서만 호출 가능하다(check(IsInGameThread())).
class FOBSProcessController
{
public:
	explicit FOBSProcessController(TSharedRef<IOBSProcessPlatform> InPlatform);
	~FOBSProcessController();

	void Initialize();
	void Shutdown();

	// OBS 실행을 요청한다. 이미 실행 중(Owned/External)이거나 진행 중이면 무시한다.
	void StartOBS();

	// 종료 준비 상태로 전환한다. RunningOwned에서만 유효하다.
	void MarkPreparingShutdown();

	// 종료 준비를 취소하고 RunningOwned로 되돌린다. PreparingShutdown에서만 유효하다.
	void CancelPreparingShutdown();

	// 소유 OBS에 정상 종료를 요청한다. PreparingShutdown에서만 유효하다.
	void RequestCloseOwnedOBS();

	// 사용자가 설정에서 실행 경로를 바꾸면 Module이 호출한다. 실행 중인 프로세스에는 영향이 없다.
	void RefreshExecutablePathCache();

	EOBSProcessState GetState() const { return State; }
	bool IsOwnedProcessRunning() const;
	FString GetStateDescription() const;

	FOnOBSProcessStateChanged& OnStateChanged() { return StateChangedEvent; }

private:
	// Core Ticker에서 0.5~1초 간격으로 호출된다.
	bool TickMonitor(float DeltaTime);

	FString ResolveExecutablePath() const;
	FString BuildLaunchArguments() const;

	void SetState(EOBSProcessState NewState);

	// Starting -> Failed 전환 시 Handle을 먼저 해제하고 그다음 상태를 바꾼다(§6.2).
	void FailStarting(const FString& Reason);

	TSharedRef<IOBSProcessPlatform> Platform;

	EOBSProcessState State = EOBSProcessState::Stopped;

	FOBSProcessInfo OwnedProcess;
	FOBSProcessInfo ExternalProcess;

	double LaunchRequestedTime = 0.0;
	double CloseRequestedTime = 0.0;

	FString CachedExecutablePath;
	FString LastFailureReason;

	FTSTicker::FDelegateHandle MonitorTickerHandle;

	FOnOBSProcessStateChanged StateChangedEvent;
};
