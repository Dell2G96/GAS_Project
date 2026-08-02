#include "PIERecordingCoordinator.h"

#include "PIEAutoRecorderLog.h"
#include "PIEAutoRecorderSettings.h"
#include "Disposition/RecordingDispositionQueue.h"
#include "PIEAutoRecorderNotification.h"
#include "Editor.h"
#include "Engine/World.h"
#include "HAL/PlatformProcess.h"

namespace
{
	// 에디터 종료 시 stop 응답을 기다리는 최대 시간(초). 이보다 오래 붙잡지 않는다.
	constexpr double MaxExitWaitSeconds = 2.0;

	// 종료 대기 중 엔진을 돌려 주는 간격(초).
	constexpr float ExitWaitTickInterval = 0.02f;
}

// 백엔드와 저장 대기열을 받아 둔다. 델리게이트 등록은 모듈이 한다.
FPIERecordingCoordinator::FPIERecordingCoordinator(TSharedRef<IPIERecordingBackend> InBackend, TSharedRef<FRecordingDispositionQueue> InDisposition)
	: Backend(InBackend)
	, Disposition(InDisposition)
{
}

// 예약해 둔 지연 stop을 반드시 해제한다. 남겨 두면 모듈 언로드 후 호출되어 크래시한다.
FPIERecordingCoordinator::~FPIERecordingCoordinator()
{
	CancelStopDelay();
}

// 현재 상태를 한국어 문자열로 돌려준다.
FString FPIERecordingCoordinator::GetStateDescription() const
{
	switch (State)
	{
	case EPIERecordingState::Idle:				return TEXT("대기");
	case EPIERecordingState::Querying:			return TEXT("상태 조회 중");
	case EPIERecordingState::StartPending:		return TEXT("시작 응답 대기");
	case EPIERecordingState::RecordingOwned:	return TEXT("녹화 중 (우리 소유)");
	case EPIERecordingState::StopPending:		return TEXT("정지 응답 대기");
	case EPIERecordingState::ExternalRecording:	return TEXT("외부 녹화 중 (소유권 없음)");
	case EPIERecordingState::Failed:			return TEXT("실패");
	default:									return TEXT("알 수 없음");
	}
}

// 녹화나 요청이 진행 중이면 true. 이때 연결을 끊으면 정지 요청을 보낼 수 없게 된다.
bool FPIERecordingCoordinator::IsBusy() const
{
	return State == EPIERecordingState::Querying
		|| State == EPIERecordingState::StartPending
		|| State == EPIERecordingState::RecordingOwned
		|| State == EPIERecordingState::StopPending;
}

// PIE가 시작됐다. 세션 ID를 새로 발급하고 상태 조회부터 시작한다.
void FPIERecordingCoordinator::HandlePostPIEStarted(bool bIsSimulating)
{
	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	if (Settings == nullptr || !Settings->bEnableAutoRecording)
	{
		return;
	}

	if (bIsSimulating && !Settings->bRecordSimulateInEditor)
	{
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("Simulate In Editor는 녹화 대상이 아닙니다. (설정으로 켤 수 있습니다)"));
		return;
	}

	// 이전 세션이 정리되지 않은 채로 새 PIE가 시작됐다면 기록해 둔다. 상태는 아래에서 정상 경로로 처리한다.
	if (State == EPIERecordingState::RecordingOwned || State == EPIERecordingState::StartPending || State == EPIERecordingState::Querying)
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("이전 세션이 정리되지 않은 상태에서 새 PIE가 시작됐습니다. (상태: %s)"), *GetStateDescription());
	}

	CurrentSessionId = FGuid::NewGuid();
	CurrentLevelName = GetCurrentLevelName();
	SessionStartTime = FPlatformTime::Seconds();
	bStopRequestedBeforeStartDone = false;

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("PIE 시작. (세션=%s, 레벨=%s)"),
		*CurrentSessionId.ToString(EGuidFormats::DigitsWithHyphens).Left(8), *CurrentLevelName);

	// 이전 stop이 아직 진행 중이면 지금 시작하지 않는다. 상태를 추측하지 않고 stop이 끝난 뒤 다시 조회한다.
	if (State == EPIERecordingState::StopPending)
	{
		bQueryAfterStopCompletes = true;
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("이전 녹화 정지가 진행 중입니다. 완료 후 상태를 다시 조회합니다."));
		return;
	}

	BeginQuery();
}

// 상태 조회를 시작한다.
void FPIERecordingCoordinator::BeginQuery()
{
	if (!Backend->IsReady())
	{
		// 연결이 없으면 이번 세션은 녹화하지 않는다. PIE 자체에는 아무 영향이 없다.
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("OBS에 연결되어 있지 않아 이번 PIE는 녹화하지 않습니다."));
		State = EPIERecordingState::Failed;

		// 다음 세션을 위해 한 번 재연결을 시도해 둔다.
		Backend->Connect();
		return;
	}

	State = EPIERecordingState::Querying;

	Backend->QueryRecordStatus(CurrentSessionId,
		FOnRecordStatusResult::CreateRaw(this, &FPIERecordingCoordinator::OnRecordStatusReceived));
}

// 상태 조회 결과를 받았다. 여기서 소유권 분기가 결정된다.
void FPIERecordingCoordinator::OnRecordStatusReceived(bool bSuccess, const FOBSRecordStatus& Status, FGuid ForSession)
{
	// 늦게 도착한 이전 세션의 응답은 현재 상태를 건드리지 못한다.
	if (!IsCurrentSession(ForSession))
	{
		UE_LOG(LogPIEAutoRecorder, Verbose, TEXT("이전 세션의 상태 응답을 무시합니다."));
		return;
	}

	if (State != EPIERecordingState::Querying)
	{
		return;
	}

	if (!bSuccess)
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("녹화 상태를 확인하지 못했습니다. 이번 PIE는 녹화하지 않습니다."));
		State = EPIERecordingState::Failed;
		return;
	}

	// 우리가 시작하지 않은 녹화가 돌고 있다. 이 상태에는 StopRecord 경로가 없다.
	if (Status.bOutputActive)
	{
		State = EPIERecordingState::ExternalRecording;
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("OBS가 이미 녹화 중이므로 기존 녹화를 유지합니다."));
		PIEAutoRecorderNotification::ShowFailure(TEXT("OBS가 이미 녹화 중이므로 기존 녹화를 유지합니다."));
		return;
	}

	// 조회 중에 PIE가 끝났다면 시작하지 않는다.
	if (bStopRequestedBeforeStartDone)
	{
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("조회 중에 PIE가 종료되어 녹화를 시작하지 않습니다."));
		State = EPIERecordingState::Idle;
		ResetSession();
		return;
	}

	RequestStart();
}

// 녹화 시작을 요청한다. 아직 소유자가 아니다.
void FPIERecordingCoordinator::RequestStart()
{
	State = EPIERecordingState::StartPending;

	Backend->StartRecording(CurrentSessionId,
		FOnSimpleResult::CreateRaw(this, &FPIERecordingCoordinator::OnStartResult));
}

// 시작 응답을 받았다. 성공했을 때만 소유권을 갖는다.
void FPIERecordingCoordinator::OnStartResult(bool bSuccess, const FString& Error, FGuid ForSession)
{
	if (!IsCurrentSession(ForSession))
	{
		UE_LOG(LogPIEAutoRecorder, Verbose, TEXT("이전 세션의 시작 응답을 무시합니다."));
		return;
	}

	if (State != EPIERecordingState::StartPending)
	{
		return;
	}

	if (!bSuccess)
	{
		// 소유권이 없으므로 아무것도 정지하지 않는다.
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("녹화를 시작하지 못했습니다. %s"),
			Error.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("(%s)"), *Error));

		State = bStopRequestedBeforeStartDone ? EPIERecordingState::Idle : EPIERecordingState::Failed;
		if (bStopRequestedBeforeStartDone)
		{
			ResetSession();
		}
		return;
	}

	// 여기서만 소유자가 된다. "요청을 보냈다"로는 절대 진입하지 않는다.
	OwnedRecordingSessionId = ForSession;
	State = EPIERecordingState::RecordingOwned;

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("PIE 녹화를 시작했습니다. (세션=%s)"),
		*ForSession.ToString(EGuidFormats::DigitsWithHyphens).Left(8));
	PIEAutoRecorderNotification::ShowSuccess(TEXT("PIE 녹화를 시작했습니다."));

	// 시작 응답 전에 PIE가 끝났다면 이제 이어서 정지한다. start와 stop을 동시에 in-flight로 두지 않기 위함이다.
	if (bStopRequestedBeforeStartDone)
	{
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("시작 응답 전에 PIE가 종료되어 이어서 정지합니다."));
		RequestStop();
	}
}

// PIE가 끝나려 한다. 소유한 녹화만 정지한다.
void FPIERecordingCoordinator::HandlePrePIEEnded(bool bIsSimulating)
{
	switch (State)
	{
	case EPIERecordingState::ExternalRecording:
		// 남의 녹화다. 아무것도 하지 않는다.
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("외부 녹화 세션이므로 정지하지 않습니다."));
		State = EPIERecordingState::Idle;
		ResetSession();
		break;

	case EPIERecordingState::Querying:
	case EPIERecordingState::StartPending:
		// 아직 소유자가 아니다. 응답이 온 뒤에 이어서 처리한다.
		bStopRequestedBeforeStartDone = true;
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("응답 대기 중에 PIE가 종료됐습니다. 응답 후 처리합니다."));
		break;

	case EPIERecordingState::RecordingOwned:
		if (OwnedRecordingSessionId != CurrentSessionId)
		{
			UE_LOG(LogPIEAutoRecorder, Warning, TEXT("소유 세션과 현재 세션이 다릅니다. 정지하지 않습니다."));
			break;
		}
		RequestStop();
		break;

	default:
		break;
	}
}

// 정지를 요청한다. 지연 설정이 있으면 그만큼 늦춘다.
void FPIERecordingCoordinator::RequestStop()
{
	State = EPIERecordingState::StopPending;
	StoppingSessionId = OwnedRecordingSessionId;

	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	const float Delay = Settings ? Settings->StopDelaySeconds : 0.0f;

	if (Delay <= 0.0f)
	{
		SendStopNow();
		return;
	}

	// 월드 타이머가 아니라 FTSTicker를 쓴다. PIE 월드는 종료 시 사라지기 때문이다.
	CancelStopDelay();
	StopDelayTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FPIERecordingCoordinator::TickStopDelay), Delay);
}

// 지연 시간이 지나 실제로 정지를 보낸다.
bool FPIERecordingCoordinator::TickStopDelay(float DeltaTime)
{
	StopDelayTickerHandle.Reset();
	SendStopNow();

	return false;
}

// 예약된 지연 정지를 취소한다.
void FPIERecordingCoordinator::CancelStopDelay()
{
	if (StopDelayTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(StopDelayTickerHandle);
		StopDelayTickerHandle.Reset();
	}
}

// StopRecord를 전송한다.
void FPIERecordingCoordinator::SendStopNow()
{
	if (!StoppingSessionId.IsValid())
	{
		return;
	}

	Backend->StopRecording(StoppingSessionId,
		FOnStopResult::CreateRaw(this, &FPIERecordingCoordinator::OnStopResult));
}

// 정지 응답을 받았다. 4단계에서는 경로를 로그로만 남기고 파일은 건드리지 않는다.
void FPIERecordingCoordinator::OnStopResult(bool bSuccess, const FString& OutputPath, FGuid ForSession)
{
	// stop은 이전 세션 ID로 보냈으므로 CurrentSessionId가 아니라 StoppingSessionId와 비교한다.
	if (ForSession != StoppingSessionId)
	{
		UE_LOG(LogPIEAutoRecorder, Verbose, TEXT("이전 세션의 정지 응답을 무시합니다."));
		return;
	}

	const double DurationSeconds = FPlatformTime::Seconds() - SessionStartTime;

	StoppingSessionId.Invalidate();
	OwnedRecordingSessionId.Invalidate();
	bWaitingStopAtExit = false;

	if (bSuccess)
	{
		// 5단계에서 이 값이 저장 확인 창으로 넘어간다. 지금은 로그로만 확인한다.
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("PIE 녹화를 종료했습니다. (레벨=%s, 길이=%.1f초)"), *CurrentLevelName, DurationSeconds);
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("녹화 파일: %s"), OutputPath.IsEmpty() ? TEXT("(경로 없음)") : *OutputPath);

		if (OutputPath.IsEmpty())
		{
			UE_LOG(LogPIEAutoRecorder, Warning, TEXT("정지는 성공했으나 outputPath가 비어 있어 저장 처리를 건너뜁니다."));
		}
		else
		{
			// 우리가 시작한 녹화의 파일만 대기열로 넘어간다. ExternalRecording 세션은 여기 도달하지 않는다.
			FRecordingDispositionItem DispositionItem;
			DispositionItem.SessionId = ForSession;
			DispositionItem.OutputPath = OutputPath;
			DispositionItem.LevelName = CurrentLevelName;
			DispositionItem.DurationSeconds = DurationSeconds;

			Disposition->Enqueue(DispositionItem);
		}
	}
	else
	{
		// 정지 성공 여부가 불명확하면 조용히 넘어가지 않는다.
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("녹화 정지에 실패했습니다. OBS 상태를 직접 확인하세요."));
		PIEAutoRecorderNotification::ShowFailure(TEXT("녹화 정지에 실패했습니다. OBS 상태를 직접 확인하세요."));
	}

	State = EPIERecordingState::Idle;

	// 정지를 기다리는 동안 새 PIE가 시작됐다면 이제 조회부터 다시 한다.
	if (bQueryAfterStopCompletes && CurrentSessionId.IsValid())
	{
		bQueryAfterStopCompletes = false;
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("이전 정지가 끝나 새 세션의 상태 조회를 시작합니다."));
		BeginQuery();
	}
}

// PIE 정리가 끝났다. 응답을 기다리는 중이 아니면 세션 값을 비운다.
void FPIERecordingCoordinator::HandleShutdownPIE(bool bIsSimulating)
{
	// 응답을 기다리는 상태는 건드리지 않는다. 콜백이 스스로 정리한다.
	if (State == EPIERecordingState::StopPending
		|| State == EPIERecordingState::StartPending
		|| State == EPIERecordingState::Querying)
	{
		return;
	}

	if (State == EPIERecordingState::RecordingOwned)
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("녹화 소유 상태로 PIE가 정리됐습니다. 정지를 시도합니다."));
		RequestStop();
		return;
	}

	State = EPIERecordingState::Idle;
	ResetSession();
}

// Play가 취소됐다. 시작된 것이 없으면 그대로 정리한다.
void FPIERecordingCoordinator::HandleCancelPIE()
{
	UE_LOG(LogPIEAutoRecorder, Log, TEXT("PIE가 취소됐습니다."));

	switch (State)
	{
	case EPIERecordingState::Querying:
	case EPIERecordingState::StartPending:
		// 응답이 오면 시작하지 않고 정리하도록 표시만 해 둔다.
		bStopRequestedBeforeStartDone = true;
		break;

	case EPIERecordingState::RecordingOwned:
		RequestStop();
		break;

	case EPIERecordingState::StopPending:
		break;

	default:
		State = EPIERecordingState::Idle;
		ResetSession();
		break;
	}
}

// 에디터가 종료된다. 소유한 녹화만 정지를 최선 시도하되 무한정 기다리지 않는다.
void FPIERecordingCoordinator::HandleEditorPreExit()
{
	CancelStopDelay();

	if (State != EPIERecordingState::RecordingOwned && State != EPIERecordingState::StopPending)
	{
		return;
	}

	if (State == EPIERecordingState::RecordingOwned)
	{
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("에디터 종료 전에 녹화 정지를 시도합니다."));
		StoppingSessionId = OwnedRecordingSessionId;
		SendStopNow();
	}

	// 응답이 올 때까지 짧게만 기다린다. 에디터 종료가 이 툴 때문에 멈추면 안 된다.
	bWaitingStopAtExit = true;

	const double StartTime = FPlatformTime::Seconds();
	while (bWaitingStopAtExit && (FPlatformTime::Seconds() - StartTime) < MaxExitWaitSeconds)
	{
		// 소켓 수신과 타임아웃 감시가 ticker에서 돌아가므로 직접 돌려 준다.
		FTSTicker::GetCoreTicker().Tick(ExitWaitTickInterval);
		FPlatformProcess::Sleep(ExitWaitTickInterval);
	}

	if (bWaitingStopAtExit)
	{
		bWaitingStopAtExit = false;
		UE_LOG(LogPIEAutoRecorder, Warning,
			TEXT("%.0f초 안에 정지 응답을 받지 못했습니다. OBS가 계속 녹화 중일 수 있으니 직접 확인하세요."), MaxExitWaitSeconds);
	}
}

// 응답이 현재 세션의 것인지 확인한다.
bool FPIERecordingCoordinator::IsCurrentSession(FGuid ForSession) const
{
	return ForSession.IsValid() && ForSession == CurrentSessionId;
}

// 세션 값을 전부 비운다. 소유권도 함께 사라진다.
void FPIERecordingCoordinator::ResetSession()
{
	CurrentSessionId.Invalidate();
	OwnedRecordingSessionId.Invalidate();
	bStopRequestedBeforeStartDone = false;
	CurrentLevelName.Reset();
	SessionStartTime = 0.0;
}

// 현재 PIE 월드의 레벨 이름을 얻는다. UEDPIE 접두사는 제거한다.
FString FPIERecordingCoordinator::GetCurrentLevelName()
{
	if (GEditor == nullptr)
	{
		return TEXT("Unknown");
	}

	if (UWorld* PlayWorld = GEditor->PlayWorld)
	{
		return UWorld::RemovePIEPrefix(PlayWorld->GetMapName());
	}

	return TEXT("Unknown");
}
