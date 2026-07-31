#pragma once

#include "CoreMinimal.h"
#include "IPIERecordingBackend.h"
#include "Containers/Ticker.h"

class IWebSocket;

// obs-websocket 연결 상태.
enum class EOBSConnectionState : uint8
{
	// 연결되어 있지 않다.
	Disconnected,

	// 소켓 연결 시도 중.
	Connecting,

	// 소켓은 붙었고 서버의 Hello(op=0)를 기다린다.
	WaitingHello,

	// Identify(op=1)를 보내고 Identified(op=2)를 기다린다.
	Identifying,

	// 요청을 보낼 수 있는 상태.
	Ready,

	// 재시도를 소진했거나 인증에 실패했다. 자동으로 다시 붙지 않는다.
	Failed,
};

// 응답을 기다리는 요청 하나. requestId로 이 맵을 조회해 콜백을 실행한다.
struct FOBSPendingRequest
{
	FGuid   SessionId;
	FString RequestType;
	double  IssuedTime = 0.0;

	// 성공·실패 어느 쪽이든 정확히 한 번 호출된다.
	TFunction<void(bool /*bSuccess*/, const FOBSRequestResponse& /*Response*/)> OnComplete;
};

// obs-websocket 5.x 세션을 담당하는 백엔드.
// 연결 수명, 핸드셰이크, 요청-응답 상관관계, 타임아웃, 재연결을 책임진다.
class FOBSWebSocketBackend : public IPIERecordingBackend
{
public:
	FOBSWebSocketBackend();
	virtual ~FOBSWebSocketBackend();

	//~ IPIERecordingBackend
	virtual void Connect() override;
	virtual void Disconnect() override;
	virtual bool IsReady() const override;
	virtual void QueryRecordStatus(FGuid SessionId, FOnRecordStatusResult Callback) override;
	virtual void StartRecording(FGuid SessionId, FOnSimpleResult Callback) override;
	virtual void StopRecording(FGuid SessionId, FOnStopResult Callback) override;
	virtual FOnRecordStateChanged& OnRecordStateChanged() override { return RecordStateChangedEvent; }
	//~ End IPIERecordingBackend

	// 현재 연결 상태. 로그와 진단 명령에 쓴다.
	EOBSConnectionState GetConnectionState() const { return ConnectionState; }

	// 현재 상태를 사람이 읽을 수 있는 문자열로 돌려준다.
	FString GetStateDescription() const;

private:
	//~ 소켓 이벤트 처리
	void HandleConnected();
	void HandleConnectionError(const FString& Error);
	void HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean);
	void HandleMessage(const FString& Message);

	//~ 메시지 종류별 처리
	void ProcessHello(const TSharedPtr<FJsonObject>& Data);
	void ProcessIdentified(const TSharedPtr<FJsonObject>& Data);
	void ProcessRequestResponse(const TSharedPtr<FJsonObject>& Data);
	void ProcessEvent(const TSharedPtr<FJsonObject>& Data);

	// 요청을 보내고 응답 대기 목록에 넣는다. 보낼 수 없는 상태면 즉시 실패 콜백을 부른다.
	void SendRequest(const FString& RequestType, FGuid SessionId, TFunction<void(bool, const FOBSRequestResponse&)> OnComplete);

	// 응답이 오지 않은 요청을 제한 시간 기준으로 정리한다. 맵이 무한히 커지지 않게 한다.
	bool TickPendingRequests(float DeltaTime);

	// 대기 중인 요청을 전부 실패로 끝낸다. 연결이 끊겼을 때 호출한다.
	void FailAllPendingRequests(const FString& Reason);

	// 소켓 델리게이트를 해제하고 닫는다. 수명 관리의 핵심이므로 한 곳에만 둔다.
	void CleanupSocket();

	// 지수 백오프로 재연결을 예약한다. 최대 횟수를 넘으면 Failed로 끝낸다.
	void ScheduleReconnect();

	// 재연결 예약을 취소한다.
	void CancelReconnect();

	// 예약 시간이 되면 한 번만 실행되어 재연결을 시도한다.
	bool TickReconnect(float DeltaTime);

	// 연결·인증이 제한 시간 안에 끝나지 않으면 끊는다. OS의 TCP 타임아웃(수십 초)을 기다리지 않기 위함이다.
	void StartConnectTimeout();
	void CancelConnectTimeout();
	bool TickConnectTimeout(float DeltaTime);

	FOnRecordStateChanged RecordStateChangedEvent;

	TSharedPtr<IWebSocket> WebSocket;
	EOBSConnectionState    ConnectionState = EOBSConnectionState::Disconnected;

	TMap<FString /*requestId*/, FOBSPendingRequest> PendingRequests;

	FTSTicker::FDelegateHandle TimeoutTickerHandle;
	FTSTicker::FDelegateHandle ReconnectTickerHandle;
	FTSTicker::FDelegateHandle ConnectTimeoutTickerHandle;

	int32   ReconnectAttempt = 0;
	FString CurrentUrl;

	// 연결 실패 알림을 매번 띄우지 않기 위한 플래그. 로그는 계속 남긴다.
	bool bReportedConnectionFailure = false;
};
