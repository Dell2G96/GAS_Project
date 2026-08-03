#include "OBSProcessController.h"

#include "PIEAutoRecorderLog.h"
#include "PIEAutoRecorderSettings.h"
#include "Misc/Paths.h"

namespace
{
	// 상태 감시 주기(초). §6.2: 0.5~1초 간격.
	constexpr float MonitorTickInterval = 0.5f;
}

FOBSProcessController::FOBSProcessController(TSharedRef<IOBSProcessPlatform> InPlatform)
	: Platform(InPlatform)
{
}

FOBSProcessController::~FOBSProcessController()
{
	Shutdown();
}

// Ticker를 등록하고 실행 경로를 1회 캐시한다.
void FOBSProcessController::Initialize()
{
	check(IsInGameThread());

	RefreshExecutablePathCache();

	MonitorTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FOBSProcessController::TickMonitor), MonitorTickInterval);
}

// Ticker를 해제하고 소유 Handle을 정리한다. 프로세스 자체는 건드리지 않는다(Editor 종료 흐름이 별도로 처리).
void FOBSProcessController::Shutdown()
{
	check(IsInGameThread());

	if (MonitorTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(MonitorTickerHandle);
		MonitorTickerHandle.Reset();
	}

	if (OwnedProcess.Handle.IsValid())
	{
		Platform->Release(OwnedProcess);
	}

	if (ExternalProcess.Handle.IsValid())
	{
		Platform->Release(ExternalProcess);
	}
}

// 실행 경로 캐시를 갱신한다. 현재 실행 중인 프로세스의 소유 정보는 바꾸지 않는다(§7.4).
void FOBSProcessController::RefreshExecutablePathCache()
{
	check(IsInGameThread());

	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	CachedExecutablePath = Settings ? Settings->OBSExecutablePath.FilePath : FString();
}

FString FOBSProcessController::ResolveExecutablePath() const
{
	return CachedExecutablePath;
}

// §7.2 커맨드라인 인자 정책. 비밀번호는 절대 포함하지 않는다.
FString FOBSProcessController::BuildLaunchArguments() const
{
	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();

	TArray<FString> Args;

	if (Settings && Settings->bLaunchOBSMinimized)
	{
		Args.Add(TEXT("--minimize-to-tray"));
	}

	Args.Add(TEXT("--disable-updater"));

	if (Settings && !Settings->OBSLaunchArguments.IsEmpty())
	{
		Args.Add(Settings->OBSLaunchArguments);
	}

	return FString::Join(Args, TEXT(" "));
}

// 상태를 바꾸고 델리게이트를 발행한다.
void FOBSProcessController::SetState(EOBSProcessState NewState)
{
	State = NewState;
	StateChangedEvent.Broadcast(State);
}

// OBS 실행을 요청한다. 이미 실행 중이거나 진행 중이면 무시한다(중복 실행 방지).
void FOBSProcessController::StartOBS()
{
	check(IsInGameThread());

	if (State != EOBSProcessState::Stopped && State != EOBSProcessState::Failed)
	{
		UE_LOG(LogPIEAutoRecorder, Verbose, TEXT("[OBS Process] StartOBS 요청을 무시합니다. (현재 상태=%s)"), *GetStateDescription());
		return;
	}

	const FString ExecutablePath = ResolveExecutablePath();
	if (ExecutablePath.IsEmpty() || !FPaths::FileExists(ExecutablePath))
	{
		LastFailureReason = TEXT("OBS 실행 경로가 설정되지 않았거나 존재하지 않습니다.");
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("[OBS Process] %s"), *LastFailureReason);
		SetState(EOBSProcessState::Failed);
		return;
	}

	// 실행 전 기존 인스턴스를 먼저 탐색한다. 중복 실행을 막기 위함이다.
	FOBSProcessInfo Found = Platform->FindRunningOBS(ExecutablePath);
	if (Found.IsValid())
	{
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("[OBS Process] 외부 OBS 발견 (PID=%u)"), Found.ProcessId);
		ExternalProcess = Found;
		SetState(EOBSProcessState::RunningExternal);
		return;
	}

	const FString WorkingDirectory = FPaths::GetPath(ExecutablePath);
	FOBSProcessInfo Launched = Platform->LaunchOBS(ExecutablePath, BuildLaunchArguments(), WorkingDirectory);
	if (!Launched.IsValid())
	{
		LastFailureReason = TEXT("OBS 실행에 실패했습니다. 실행 파일과 권한을 확인하세요.");
		SetState(EOBSProcessState::Failed);
		return;
	}

	OwnedProcess = Launched;
	LaunchRequestedTime = FPlatformTime::Seconds();
	UE_LOG(LogPIEAutoRecorder, Log, TEXT("[OBS Process] 소유 OBS 실행 시도. (PID=%u)"), OwnedProcess.ProcessId);
	SetState(EOBSProcessState::Starting);
}

// RunningOwned -> PreparingShutdown. Module이 녹화 정리를 진행하는 동안 UI를 잠그는 용도.
void FOBSProcessController::MarkPreparingShutdown()
{
	check(IsInGameThread());

	if (State != EOBSProcessState::RunningOwned)
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("[OBS Process] 잘못된 전이: MarkPreparingShutdown (현재 상태=%s)"), *GetStateDescription());
		return;
	}

	SetState(EOBSProcessState::PreparingShutdown);
}

// PreparingShutdown -> RunningOwned. 종료 준비가 거부됐을 때 복원한다.
void FOBSProcessController::CancelPreparingShutdown()
{
	check(IsInGameThread());

	if (State != EOBSProcessState::PreparingShutdown)
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("[OBS Process] 잘못된 전이: CancelPreparingShutdown (현재 상태=%s)"), *GetStateDescription());
		return;
	}

	SetState(EOBSProcessState::RunningOwned);
}

// PreparingShutdown -> Closing. 소유 OBS에만 정상 종료 메시지를 보낸다.
void FOBSProcessController::RequestCloseOwnedOBS()
{
	check(IsInGameThread());

	if (State != EOBSProcessState::PreparingShutdown)
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("[OBS Process] 잘못된 전이: RequestCloseOwnedOBS (현재 상태=%s)"), *GetStateDescription());
		return;
	}

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("[OBS Process] 정상 종료 요청. (PID=%u)"), OwnedProcess.ProcessId);

	if (!Platform->RequestGracefulClose(OwnedProcess))
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("[OBS Process] 정상 종료 메시지 전송에 실패했습니다."));
		SetState(EOBSProcessState::RunningOwned);
		return;
	}

	CloseRequestedTime = FPlatformTime::Seconds();
	SetState(EOBSProcessState::Closing);
}

bool FOBSProcessController::IsOwnedProcessRunning() const
{
	return State == EOBSProcessState::RunningOwned && Platform->IsProcessRunning(OwnedProcess);
}

FString FOBSProcessController::GetStateDescription() const
{
	switch (State)
	{
	case EOBSProcessState::Stopped:            return TEXT("정지됨");
	case EOBSProcessState::Starting:           return TEXT("시작 중");
	case EOBSProcessState::RunningOwned:       return TEXT("실행 중 (소유)");
	case EOBSProcessState::RunningExternal:    return TEXT("실행 중 (외부)");
	case EOBSProcessState::PreparingShutdown:  return TEXT("녹화 정리 중");
	case EOBSProcessState::Closing:            return TEXT("종료 중");
	case EOBSProcessState::Failed:             return LastFailureReason.IsEmpty() ? TEXT("실패") : FString::Printf(TEXT("실패 (%s)"), *LastFailureReason);
	default:                                   return TEXT("알 수 없음");
	}
}

// Starting -> Failed 전환. Handle을 먼저 해제한 뒤 상태를 바꾼다(§6.2 Handle 정리 순서).
void FOBSProcessController::FailStarting(const FString& Reason)
{
	LastFailureReason = Reason;

	if (OwnedProcess.Handle.IsValid())
	{
		Platform->Release(OwnedProcess);
	}
	OwnedProcess = FOBSProcessInfo();

	SetState(EOBSProcessState::Failed);
}

// Core Ticker 기반 상태 감시. §6.2에 명시된 전이만 수행한다.
bool FOBSProcessController::TickMonitor(float DeltaTime)
{
	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	const double StartupTimeout = Settings ? static_cast<double>(Settings->OBSStartupTimeoutSeconds) : 10.0;
	const double ShutdownTimeout = Settings ? static_cast<double>(Settings->OBSShutdownTimeoutSeconds) : 10.0;

	switch (State)
	{
	case EOBSProcessState::Starting:
		{
			if (!Platform->IsProcessRunning(OwnedProcess))
			{
				UE_LOG(LogPIEAutoRecorder, Warning, TEXT("[OBS Process] 실행 직후 종료됨."));
				FailStarting(TEXT("실행 직후 프로세스가 종료됨"));
				break;
			}

			if (FPlatformTime::Seconds() - LaunchRequestedTime >= StartupTimeout)
			{
				UE_LOG(LogPIEAutoRecorder, Log, TEXT("[OBS Process] 소유 OBS 실행 성공."));
				SetState(EOBSProcessState::RunningOwned);
			}
			break;
		}

	case EOBSProcessState::RunningOwned:
		{
			if (!Platform->IsProcessRunning(OwnedProcess))
			{
				UE_LOG(LogPIEAutoRecorder, Log, TEXT("[OBS Process] 소유 OBS가 예상치 못하게 종료됨."));
				Platform->Release(OwnedProcess);
				OwnedProcess = FOBSProcessInfo();
				SetState(EOBSProcessState::Stopped);
			}
			break;
		}

	case EOBSProcessState::RunningExternal:
		{
			if (!Platform->IsProcessRunning(ExternalProcess))
			{
				UE_LOG(LogPIEAutoRecorder, Log, TEXT("[OBS Process] 외부 OBS 종료 감지."));
				Platform->Release(ExternalProcess);
				ExternalProcess = FOBSProcessInfo();
				SetState(EOBSProcessState::Stopped);
			}
			break;
		}

	case EOBSProcessState::PreparingShutdown:
		{
			// 이 상태에서 프로세스가 스스로 종료되면 종료 준비를 취소하고 Stopped로 간다.
			if (!Platform->IsProcessRunning(OwnedProcess))
			{
				UE_LOG(LogPIEAutoRecorder, Log, TEXT("[OBS Process] 종료 준비 중 소유 OBS가 스스로 종료됨."));
				Platform->Release(OwnedProcess);
				OwnedProcess = FOBSProcessInfo();
				SetState(EOBSProcessState::Stopped);
			}
			break;
		}

	case EOBSProcessState::Closing:
		{
			if (!Platform->IsProcessRunning(OwnedProcess))
			{
				UE_LOG(LogPIEAutoRecorder, Log, TEXT("[OBS Process] 정상 종료 완료."));
				Platform->Release(OwnedProcess);
				OwnedProcess = FOBSProcessInfo();
				SetState(EOBSProcessState::Stopped);
				break;
			}

			if (FPlatformTime::Seconds() - CloseRequestedTime >= ShutdownTimeout)
			{
				UE_LOG(LogPIEAutoRecorder, Warning, TEXT("[OBS Process] 종료 제한 시간 초과. OBS를 남겨둡니다."));
				SetState(EOBSProcessState::RunningOwned);
			}
			break;
		}

	default:
		break;
	}

	return true;
}
