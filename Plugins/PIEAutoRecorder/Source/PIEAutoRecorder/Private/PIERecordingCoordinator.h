#pragma once

#include "CoreMinimal.h"
#include "IPIERecordingBackend.h"
#include "Containers/Ticker.h"

class FRecordingDispositionQueue;

// PIE 세션과 녹화의 결합 상태.
enum class EPIERecordingState : uint8
{
	// 아무 일도 일어나지 않는 상태.
	Idle,

	// GetRecordStatus 응답을 기다린다. 아직 아무것도 시작하지 않았다.
	Querying,

	// StartRecord 응답을 기다린다. 아직 소유자가 아니다.
	StartPending,

	// StartRecord 성공 응답을 받았다. 이 상태에서만 우리가 소유자다.
	RecordingOwned,

	// StopRecord 응답을 기다린다.
	StopPending,

	// 우리가 시작하지 않은 녹화가 이미 돌고 있다. StopRecord 경로가 아예 없다.
	ExternalRecording,

	// 이번 세션은 실패했다. 다음 세션에 영향을 주지 않는다.
	Failed,
};

// PIE 세션 상태와 녹화 백엔드 상태를 결합하는 조정자.
// 녹화 소유권 판정과 요청 직렬화를 책임진다. WebSocket이나 JSON은 전혀 모른다.
class FPIERecordingCoordinator
{
public:
	FPIERecordingCoordinator(TSharedRef<IPIERecordingBackend> InBackend, TSharedRef<FRecordingDispositionQueue> InDisposition);
	~FPIERecordingCoordinator();

	void HandlePostPIEStarted(bool bIsSimulating);
	void HandlePrePIEEnded(bool bIsSimulating);
	void HandleShutdownPIE(bool bIsSimulating);
	void HandleCancelPIE();
	void HandleEditorPreExit();

	// 진단용. 콘솔 명령에서 현재 상태를 보여준다.
	FString GetStateDescription() const;

	// 녹화나 요청이 진행 중인지. 설정 변경으로 연결을 끊어도 되는지 판단하는 데 쓴다.
	bool IsBusy() const;

private:
	// GetRecordStatus부터 다시 시작한다. 상태를 추측하지 않고 매번 확인한다.
	void BeginQuery();

	void RequestStart();

	// StopRecord를 보낸다. StopDelaySeconds가 0보다 크면 그만큼 늦춘다.
	void RequestStop();
	void SendStopNow();
	bool TickStopDelay(float DeltaTime);
	void CancelStopDelay();

	void OnRecordStatusReceived(bool bSuccess, const FOBSRecordStatus& Status, FGuid ForSession);
	void OnStartResult(bool bSuccess, const FString& Error, FGuid ForSession);
	void OnStopResult(bool bSuccess, const FString& OutputPath, FGuid ForSession);

	// 늦게 도착한 응답이 현재 세션 상태를 바꾸지 못하게 막는 검사.
	bool IsCurrentSession(FGuid ForSession) const;

	// 세션 관련 값을 전부 비운다. 소유권은 여기서 함께 사라진다.
	void ResetSession();

	// 현재 PIE 레벨 이름을 얻는다. 저장 창 기본 파일명에 쓰기 위해 미리 보관해 둔다.
	static FString GetCurrentLevelName();

	TSharedRef<IPIERecordingBackend>       Backend;
	TSharedRef<FRecordingDispositionQueue> Disposition;

	EPIERecordingState State = EPIERecordingState::Idle;

	// 지금 진행 중인 PIE 세션의 ID. PostPIEStarted마다 새로 발급한다.
	FGuid CurrentSessionId;

	// StartRecord가 실제로 성공 응답을 받은 세션의 ID. 유효할 때만 우리가 소유자다.
	FGuid OwnedRecordingSessionId;

	// StopRecord를 보낸 세션의 ID. 늦은 stop 응답을 구분하는 데 쓴다.
	FGuid StoppingSessionId;

	// 시작 응답이 오기 전에 PIE가 끝났다. start 성공 시 이어서 stop한다.
	bool bStopRequestedBeforeStartDone = false;

	// 이전 stop이 끝나면 새 세션의 조회를 시작해야 한다.
	bool bQueryAfterStopCompletes = false;

	// 에디터 종료 중 stop 응답을 기다리는 동안만 true.
	bool bWaitingStopAtExit = false;

	FString CurrentLevelName;
	double  SessionStartTime = 0.0;

	FTSTicker::FDelegateHandle StopDelayTickerHandle;
};
