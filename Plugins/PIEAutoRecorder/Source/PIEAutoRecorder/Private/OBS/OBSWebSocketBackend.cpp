#include "OBSWebSocketBackend.h"

#include "PIEAutoRecorderLog.h"
#include "PIEAutoRecorderSettings.h"
#include "PIEAutoRecorderNotification.h"
#include "IWebSocket.h"
#include "WebSocketsModule.h"
#include "Dom/JsonObject.h"
#include "Modules/ModuleManager.h"

namespace
{
	// 대기 중인 요청을 훑는 주기(초). 타임아웃 판정 정밀도가 이 값만큼 거칠어진다.
	constexpr float TimeoutCheckInterval = 0.25f;

	// 재연결 기본 대기 시간(초). 시도할 때마다 2배씩 늘어난다.
	constexpr float BaseReconnectDelay = 1.0f;

	// 요청 이름. 계획서 원칙 4번에 따라 ToggleRecord는 어디에도 존재하지 않는다.
	const TCHAR* RequestGetRecordStatus = TEXT("GetRecordStatus");
	const TCHAR* RequestStartRecord = TEXT("StartRecord");
	const TCHAR* RequestStopRecord = TEXT("StopRecord");
}

// 타임아웃 감시 ticker를 등록한다. 연결은 하지 않는다.
FOBSWebSocketBackend::FOBSWebSocketBackend()
{
	TimeoutTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FOBSWebSocketBackend::TickPendingRequests),
		TimeoutCheckInterval);
}

// 등록한 ticker와 소켓을 대칭적으로 해제한다. 하나라도 빠지면 에디터 종료나 핫 리로드에서 크래시한다.
FOBSWebSocketBackend::~FOBSWebSocketBackend()
{
	if (TimeoutTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TimeoutTickerHandle);
		TimeoutTickerHandle.Reset();
	}

	CancelReconnect();
	CancelConnectTimeout();
	FailAllPendingRequests(TEXT("백엔드가 종료되었습니다."));
	CleanupSocket();
}

// OBS에 연결한다. 자동 녹화가 꺼져 있으면 연결 시도조차 하지 않는다.
void FOBSWebSocketBackend::Connect()
{
	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	if (Settings == nullptr)
	{
		return;
	}

	// 계획서 §14: bEnableAutoRecording=false면 완전 무동작이어야 한다.
	if (!Settings->bEnableAutoRecording)
	{
		UE_LOG(LogPIEAutoRecorder, Verbose, TEXT("자동 녹화가 꺼져 있어 OBS 연결을 시도하지 않습니다."));
		return;
	}

	// 이미 연결되어 있거나 진행 중이면 중복 연결하지 않는다.
	if (ConnectionState == EOBSConnectionState::Connecting
		|| ConnectionState == EOBSConnectionState::WaitingHello
		|| ConnectionState == EOBSConnectionState::Identifying
		|| ConnectionState == EOBSConnectionState::Ready)
	{
		return;
	}

	CancelReconnect();
	CleanupSocket();

	CurrentUrl = FString::Printf(TEXT("ws://%s:%d"), *Settings->ServerHost, Settings->ServerPort);

	FWebSocketsModule& WebSocketsModule = FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));
	WebSocket = WebSocketsModule.CreateWebSocket(CurrentUrl, FString());
	if (!WebSocket.IsValid())
	{
		UE_LOG(LogPIEAutoRecorder, Error, TEXT("WebSocket 객체를 만들지 못했습니다. (%s)"), *CurrentUrl);
		ConnectionState = EOBSConnectionState::Failed;
		return;
	}

	WebSocket->OnConnected().AddRaw(this, &FOBSWebSocketBackend::HandleConnected);
	WebSocket->OnConnectionError().AddRaw(this, &FOBSWebSocketBackend::HandleConnectionError);
	WebSocket->OnClosed().AddRaw(this, &FOBSWebSocketBackend::HandleClosed);
	WebSocket->OnMessage().AddRaw(this, &FOBSWebSocketBackend::HandleMessage);

	ConnectionState = EOBSConnectionState::Connecting;
	StartConnectTimeout();

	// 비밀번호는 설정 여부만 남긴다. 값은 어떤 경우에도 출력하지 않는다.
	UE_LOG(LogPIEAutoRecorder, Log, TEXT("OBS에 연결을 시도합니다. (%s, Password=%s)"),
		*CurrentUrl,
		Settings->ResolvePassword().IsEmpty() ? TEXT("(없음)") : TEXT("(설정됨)"));

	WebSocket->Connect();
}

// 사용자가 명시적으로 끊는다. 재연결을 예약하지 않는다.
void FOBSWebSocketBackend::Disconnect()
{
	CancelReconnect();
	CancelConnectTimeout();
	FailAllPendingRequests(TEXT("연결이 해제되었습니다."));
	CleanupSocket();

	ConnectionState = EOBSConnectionState::Disconnected;
	ReconnectAttempt = 0;
	bReportedConnectionFailure = false;

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("OBS 연결을 해제했습니다."));
}

// 요청을 보낼 수 있는 상태인지 알려준다. Identified까지 끝나야 true다.
bool FOBSWebSocketBackend::IsReady() const
{
	return ConnectionState == EOBSConnectionState::Ready && WebSocket.IsValid() && WebSocket->IsConnected();
}

// 현재 상태를 한국어 문자열로 돌려준다.
FString FOBSWebSocketBackend::GetStateDescription() const
{
	switch (ConnectionState)
	{
	case EOBSConnectionState::Disconnected:	return TEXT("연결 안 됨");
	case EOBSConnectionState::Connecting:	return TEXT("연결 시도 중");
	case EOBSConnectionState::WaitingHello:	return TEXT("Hello 대기 중");
	case EOBSConnectionState::Identifying:	return TEXT("인증 중");
	case EOBSConnectionState::Ready:		return TEXT("준비됨");
	case EOBSConnectionState::Failed:		return TEXT("실패");
	default:								return TEXT("알 수 없음");
	}
}

// 소켓이 붙었다. 아직 요청을 보낼 수 없고 서버의 Hello를 기다려야 한다.
void FOBSWebSocketBackend::HandleConnected()
{
	ConnectionState = EOBSConnectionState::WaitingHello;
	bReportedConnectionFailure = false;

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("OBS 소켓이 연결되었습니다. Hello를 기다립니다."));
}

// 연결 자체가 실패했다. OBS가 꺼져 있거나 WebSocket 서버가 비활성인 경우가 대부분이다.
void FOBSWebSocketBackend::HandleConnectionError(const FString& Error)
{
	CancelConnectTimeout();

	// 같은 실패를 반복해서 크게 남기지 않는다. 첫 번째만 Warning, 이후는 Verbose.
	if (!bReportedConnectionFailure)
	{
		UE_LOG(LogPIEAutoRecorder, Warning,
			TEXT("OBS에 연결하지 못했습니다. WebSocket 서버와 포트를 확인하세요. (%s) 사유: %s"),
			*CurrentUrl, *Error);

		// 재연결마다 알림을 띄우면 화면을 뒤덮는다. 실패 구간마다 한 번만 알린다.
		PIEAutoRecorderNotification::ShowFailure(TEXT("OBS에 연결하지 못했습니다. WebSocket 서버와 포트를 확인하세요."));
		bReportedConnectionFailure = true;
	}
	else
	{
		UE_LOG(LogPIEAutoRecorder, Verbose, TEXT("OBS 재연결 실패: %s"), *Error);
	}

	FailAllPendingRequests(TEXT("연결 실패"));
	ConnectionState = EOBSConnectionState::Disconnected;
	ScheduleReconnect();
}

// 연결이 닫혔다. 녹화 중이었다면 OBS는 계속 녹화하고 있을 수 있다.
void FOBSWebSocketBackend::HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	CancelConnectTimeout();

	UE_LOG(LogPIEAutoRecorder, Warning, TEXT("OBS 연결이 끊겼습니다. (코드=%d, 사유=%s, 정상종료=%s)"),
		StatusCode, *Reason, bWasClean ? TEXT("예") : TEXT("아니오"));

	// Identify를 보낸 직후 끊겼다면 인증 실패로 본다. 비밀번호를 고치기 전에는 재시도해도 결과가 같다.
	const bool bAuthenticationFailed = (ConnectionState == EOBSConnectionState::Identifying);

	FailAllPendingRequests(TEXT("연결이 끊겼습니다."));

	if (bAuthenticationFailed)
	{
		// 비밀번호 값은 어떤 경우에도 로그에 남기지 않는다.
		UE_LOG(LogPIEAutoRecorder, Error, TEXT("OBS 인증에 실패했습니다. 비밀번호를 확인하세요."));
		PIEAutoRecorderNotification::ShowFailure(TEXT("OBS 인증에 실패했습니다. 비밀번호를 확인하세요."));

		ConnectionState = EOBSConnectionState::Failed;
		CancelReconnect();
		return;
	}

	ConnectionState = EOBSConnectionState::Disconnected;
	ScheduleReconnect();
}

// 수신 메시지를 op별로 나눠 처리한다. 알 수 없는 메시지는 무시한다.
void FOBSWebSocketBackend::HandleMessage(const FString& Message)
{
	int32 OpCode = -1;
	TSharedPtr<FJsonObject> Data;
	if (!FOBSWebSocketProtocol::ParseEnvelope(Message, OpCode, Data))
	{
		return;
	}

	switch (static_cast<EOBSOpCode>(OpCode))
	{
	case EOBSOpCode::Hello:
		ProcessHello(Data);
		break;

	case EOBSOpCode::Identified:
		ProcessIdentified(Data);
		break;

	case EOBSOpCode::RequestResponse:
		ProcessRequestResponse(Data);
		break;

	case EOBSOpCode::Event:
		ProcessEvent(Data);
		break;

	default:
		UE_LOG(LogPIEAutoRecorder, Verbose, TEXT("처리하지 않는 op를 받았습니다. (op=%d)"), OpCode);
		break;
	}
}

// Hello를 받아 Identify를 조립해 보낸다. 인증 필요 여부는 Hello가 결정한다.
void FOBSWebSocketBackend::ProcessHello(const TSharedPtr<FJsonObject>& Data)
{
	FOBSHelloMessage Hello;
	if (!FOBSWebSocketProtocol::ParseHello(Data, Hello))
	{
		UE_LOG(LogPIEAutoRecorder, Error, TEXT("Hello를 해석하지 못했습니다."));
		ConnectionState = EOBSConnectionState::Failed;
		CancelConnectTimeout();
		CleanupSocket();
		return;
	}

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("OBS WebSocket 버전 %s (rpcVersion=%d, 인증 %s)"),
		*Hello.ObsWebSocketVersion,
		Hello.RpcVersion,
		Hello.bAuthenticationRequired ? TEXT("필요") : TEXT("불필요"));

	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	const FString Password = Settings ? Settings->ResolvePassword() : FString();

	FString IdentifyMessage;
	const EOBSIdentifyResult Result = FOBSWebSocketProtocol::BuildIdentify(
		Hello, Password, static_cast<int32>(EOBSEventSubscription::Outputs), IdentifyMessage);

	if (Result == EOBSIdentifyResult::PasswordRequired)
	{
		// 재연결해도 결과가 같으므로 Failed로 끝낸다. 사용자가 설정을 고쳐야 한다.
		UE_LOG(LogPIEAutoRecorder, Error,
			TEXT("OBS가 인증을 요구합니다. 비밀번호를 입력하거나 OBS에서 인증을 해제하세요."));
		PIEAutoRecorderNotification::ShowFailure(TEXT("OBS가 인증을 요구합니다. 비밀번호를 입력하거나 OBS에서 인증을 해제하세요."));
		ConnectionState = EOBSConnectionState::Failed;
		CancelReconnect();
		CancelConnectTimeout();
		CleanupSocket();
		return;
	}

	ConnectionState = EOBSConnectionState::Identifying;

	// Identify 메시지 전문은 인증 문자열을 포함하므로 절대 로그로 남기지 않는다.
	WebSocket->Send(IdentifyMessage);
}

// 인증까지 끝났다. 이제부터 요청을 보낼 수 있다.
void FOBSWebSocketBackend::ProcessIdentified(const TSharedPtr<FJsonObject>& Data)
{
	CancelConnectTimeout();

	ConnectionState = EOBSConnectionState::Ready;
	ReconnectAttempt = 0;
	bReportedConnectionFailure = false;

	int32 NegotiatedVersion = 0;
	if (Data.IsValid())
	{
		double Value = 0.0;
		if (Data->TryGetNumberField(TEXT("negotiatedRpcVersion"), Value))
		{
			NegotiatedVersion = static_cast<int32>(Value);
		}
	}

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("OBS 연결 및 인증에 성공했습니다. (negotiatedRpcVersion=%d)"), NegotiatedVersion);
}

// 요청 응답을 requestId로 찾아 해당 콜백만 실행하고 목록에서 제거한다.
void FOBSWebSocketBackend::ProcessRequestResponse(const TSharedPtr<FJsonObject>& Data)
{
	FOBSRequestResponse Response;
	if (!FOBSWebSocketProtocol::ParseRequestResponse(Data, Response))
	{
		return;
	}

	FOBSPendingRequest Pending;
	if (!PendingRequests.RemoveAndCopyValue(Response.RequestId, Pending))
	{
		// 타임아웃으로 이미 정리했거나 우리가 보내지 않은 요청이다. 크래시하지 않고 무시한다.
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("알 수 없는 requestId의 응답을 받았습니다. 무시합니다. (%s)"), *Response.RequestId);
		return;
	}

	if (Response.bSuccess)
	{
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("%s 성공. (code=%d)"), *Pending.RequestType, Response.Code);
	}
	else
	{
		// OBS가 준 code와 comment를 그대로 전달한다. 우리가 추측해 바꾸지 않는다.
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("%s 실패. (code=%d, comment=%s)"),
			*Pending.RequestType, Response.Code, *Response.Comment);
	}

	if (Pending.OnComplete)
	{
		Pending.OnComplete(Response.bSuccess, Response);
	}
}

// RecordStateChanged 이벤트만 골라 브로드캐스트한다.
void FOBSWebSocketBackend::ProcessEvent(const TSharedPtr<FJsonObject>& Data)
{
	FOBSRecordStateChanged Event;
	if (!FOBSWebSocketProtocol::ParseRecordStateChanged(Data, Event))
	{
		return;
	}

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("녹화 상태 이벤트: %s (active=%s)"),
		*Event.OutputState, Event.bOutputActive ? TEXT("true") : TEXT("false"));

	RecordStateChangedEvent.Broadcast(Event);
}

// 요청을 보내고 응답 대기 목록에 넣는다.
void FOBSWebSocketBackend::SendRequest(const FString& RequestType, FGuid SessionId, TFunction<void(bool, const FOBSRequestResponse&)> OnComplete)
{
	if (!IsReady())
	{
		UE_LOG(LogPIEAutoRecorder, Warning,
			TEXT("%s를 보낼 수 없습니다. 현재 상태: %s (OBS 실행과 WebSocket 서버를 확인한 뒤 PIEAutoRecorder.Connect를 실행하세요.)"),
			*RequestType, *GetStateDescription());

		if (OnComplete)
		{
			OnComplete(false, FOBSRequestResponse());
		}
		return;
	}

	const FString RequestId = FOBSWebSocketProtocol::MakeRequestId();

	FOBSPendingRequest Pending;
	Pending.SessionId = SessionId;
	Pending.RequestType = RequestType;
	Pending.IssuedTime = FPlatformTime::Seconds();
	Pending.OnComplete = MoveTemp(OnComplete);

	PendingRequests.Add(RequestId, MoveTemp(Pending));

	UE_LOG(LogPIEAutoRecorder, Verbose, TEXT("%s 전송. (requestId=%s)"), *RequestType, *RequestId);

	WebSocket->Send(FOBSWebSocketProtocol::BuildRequest(RequestType, RequestId));
}

// 녹화 상태를 조회한다. outputActive가 소유권 판정의 근거가 된다.
void FOBSWebSocketBackend::QueryRecordStatus(FGuid SessionId, FOnRecordStatusResult Callback)
{
	SendRequest(RequestGetRecordStatus, SessionId,
		[Callback, SessionId](bool bSuccess, const FOBSRequestResponse& Response)
		{
			FOBSRecordStatus Status;

			// 요청이 성공했더라도 응답 내용을 해석하지 못하면 실패로 본다. 상태를 추측하지 않는다.
			const bool bParsed = bSuccess && FOBSWebSocketProtocol::ParseRecordStatus(Response.ResponseData, Status);

			Callback.ExecuteIfBound(bParsed, Status, SessionId);
		});
}

// 녹화를 시작한다. ToggleRecord는 쓰지 않는다.
void FOBSWebSocketBackend::StartRecording(FGuid SessionId, FOnSimpleResult Callback)
{
	SendRequest(RequestStartRecord, SessionId,
		[Callback, SessionId](bool bSuccess, const FOBSRequestResponse& Response)
		{
			Callback.ExecuteIfBound(bSuccess, Response.Comment, SessionId);
		});
}

// 녹화를 정지하고 파일 경로를 받는다. 이 경로가 파일 처리의 유일한 근거다.
void FOBSWebSocketBackend::StopRecording(FGuid SessionId, FOnStopResult Callback)
{
	SendRequest(RequestStopRecord, SessionId,
		[Callback, SessionId](bool bSuccess, const FOBSRequestResponse& Response)
		{
			FString OutputPath;
			if (bSuccess)
			{
				FOBSWebSocketProtocol::ParseStopRecordOutputPath(Response.ResponseData, OutputPath);
			}

			Callback.ExecuteIfBound(bSuccess, OutputPath, SessionId);
		});
}

// 제한 시간을 넘긴 요청을 실패로 끝내고 목록에서 지운다.
bool FOBSWebSocketBackend::TickPendingRequests(float DeltaTime)
{
	if (PendingRequests.Num() == 0)
	{
		return true;
	}

	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	const double TimeoutSeconds = Settings ? Settings->RequestTimeoutSeconds : 2.0;
	const double Now = FPlatformTime::Seconds();

	// 콜백 안에서 새 요청이 추가될 수 있으므로 먼저 목록을 뽑고 나서 처리한다.
	TArray<FString> TimedOutIds;
	for (const TPair<FString, FOBSPendingRequest>& Pair : PendingRequests)
	{
		if (Now - Pair.Value.IssuedTime > TimeoutSeconds)
		{
			TimedOutIds.Add(Pair.Key);
		}
	}

	for (const FString& RequestId : TimedOutIds)
	{
		FOBSPendingRequest Pending;
		if (PendingRequests.RemoveAndCopyValue(RequestId, Pending))
		{
			UE_LOG(LogPIEAutoRecorder, Warning, TEXT("%s 응답이 %.1f초 안에 오지 않았습니다. 실패로 처리합니다."),
				*Pending.RequestType, TimeoutSeconds);

			if (Pending.OnComplete)
			{
				Pending.OnComplete(false, FOBSRequestResponse());
			}
		}
	}

	return true;
}

// 대기 중인 요청을 전부 실패로 끝낸다. 응답을 영영 못 받는 상황에서 콜백이 유실되지 않게 한다.
void FOBSWebSocketBackend::FailAllPendingRequests(const FString& Reason)
{
	if (PendingRequests.Num() == 0)
	{
		return;
	}

	UE_LOG(LogPIEAutoRecorder, Warning, TEXT("대기 중인 요청 %d건을 실패로 처리합니다. 사유: %s"), PendingRequests.Num(), *Reason);

	TMap<FString, FOBSPendingRequest> Failing = MoveTemp(PendingRequests);
	PendingRequests.Empty();

	for (TPair<FString, FOBSPendingRequest>& Pair : Failing)
	{
		if (Pair.Value.OnComplete)
		{
			Pair.Value.OnComplete(false, FOBSRequestResponse());
		}
	}
}

// 델리게이트를 먼저 해제한 뒤 소켓을 닫는다. 순서가 바뀌면 닫는 도중 콜백이 다시 들어온다.
void FOBSWebSocketBackend::CleanupSocket()
{
	if (!WebSocket.IsValid())
	{
		return;
	}

	WebSocket->OnConnected().RemoveAll(this);
	WebSocket->OnConnectionError().RemoveAll(this);
	WebSocket->OnClosed().RemoveAll(this);
	WebSocket->OnMessage().RemoveAll(this);

	if (WebSocket->IsConnected())
	{
		WebSocket->Close();
	}

	WebSocket.Reset();
}

// 지수 백오프로 재연결을 예약한다. 최대 횟수를 넘으면 더 시도하지 않는다.
void FOBSWebSocketBackend::ScheduleReconnect()
{
	CancelReconnect();

	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	if (Settings == nullptr || !Settings->bEnableAutoRecording)
	{
		return;
	}

	if (ReconnectAttempt >= Settings->MaxReconnectAttempts)
	{
		ConnectionState = EOBSConnectionState::Failed;
		UE_LOG(LogPIEAutoRecorder, Warning,
			TEXT("재연결을 %d회 시도했으나 실패했습니다. 자동 재시도를 중단합니다."), ReconnectAttempt);
		return;
	}

	// 1초 → 2초 → 4초. 매 Tick 무한 재시도를 막는다.
	const float Delay = BaseReconnectDelay * FMath::Pow(2.0f, static_cast<float>(ReconnectAttempt));
	++ReconnectAttempt;

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("%.0f초 후 재연결을 시도합니다. (%d/%d)"),
		Delay, ReconnectAttempt, Settings->MaxReconnectAttempts);

	ReconnectTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FOBSWebSocketBackend::TickReconnect), Delay);
}

// 예약된 재연결을 취소한다.
void FOBSWebSocketBackend::CancelReconnect()
{
	if (ReconnectTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ReconnectTickerHandle);
		ReconnectTickerHandle.Reset();
	}
}

// 예약 시간이 되어 재연결을 시도한다. false를 돌려주어 이 ticker는 한 번만 실행된다.
bool FOBSWebSocketBackend::TickReconnect(float DeltaTime)
{
	// Connect()가 CancelReconnect()를 호출하므로 핸들을 먼저 비워 자기 자신을 제거하지 않게 한다.
	ReconnectTickerHandle.Reset();
	Connect();

	return false;
}

// 연결 제한 시간을 건다. OS의 TCP 타임아웃은 수십 초라 이 감시가 없으면 설정값이 무의미해진다.
void FOBSWebSocketBackend::StartConnectTimeout()
{
	CancelConnectTimeout();

	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	const float Timeout = Settings ? Settings->ConnectTimeoutSeconds : 3.0f;
	if (Timeout <= 0.0f)
	{
		return;
	}

	ConnectTimeoutTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FOBSWebSocketBackend::TickConnectTimeout), Timeout);
}

// 연결 제한 시간 감시를 해제한다.
void FOBSWebSocketBackend::CancelConnectTimeout()
{
	if (ConnectTimeoutTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ConnectTimeoutTickerHandle);
		ConnectTimeoutTickerHandle.Reset();
	}
}

// 제한 시간 안에 Ready가 되지 못했다. 소켓을 정리하고 재연결 절차를 탄다.
bool FOBSWebSocketBackend::TickConnectTimeout(float DeltaTime)
{
	ConnectTimeoutTickerHandle.Reset();

	// 이미 준비됐거나 사용자가 끊었으면 할 일이 없다.
	if (ConnectionState == EOBSConnectionState::Ready
		|| ConnectionState == EOBSConnectionState::Disconnected
		|| ConnectionState == EOBSConnectionState::Failed)
	{
		return false;
	}

	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	const float Timeout = Settings ? Settings->ConnectTimeoutSeconds : 3.0f;

	UE_LOG(LogPIEAutoRecorder, Warning, TEXT("%.1f초 안에 연결이 완료되지 않았습니다. (상태: %s)"),
		Timeout, *GetStateDescription());

	// 델리게이트를 먼저 지우고 닫으므로 뒤늦은 소켓 콜백이 상태를 되돌리지 못한다.
	CleanupSocket();
	FailAllPendingRequests(TEXT("연결 시간 초과"));
	ConnectionState = EOBSConnectionState::Disconnected;
	ScheduleReconnect();

	return false;
}
