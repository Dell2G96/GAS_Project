#pragma once

#include "CoreMinimal.h"

class FJsonObject;

// obs-websocket 5.x 오피코드.
enum class EOBSOpCode : int32
{
	Hello = 0,
	Identify = 1,
	Identified = 2,
	Reidentify = 3,
	Event = 5,
	Request = 6,
	RequestResponse = 7,
};

// Identify에 넣을 이벤트 구독 비트. Outputs 계열만 최소 구독한다(RecordStateChanged 수신 목적).
enum class EOBSEventSubscription : int32
{
	None = 0,
	Outputs = 1 << 6,	// 64
};

// BuildIdentify의 결과. 실패 사유를 호출자가 구분할 수 있어야 안내 문구가 정확해진다.
enum class EOBSIdentifyResult : uint8
{
	// 정상적으로 Identify 메시지를 만들었다.
	Success,

	// 서버가 인증을 요구하는데 비밀번호가 비어 있다. 계산을 시도하지 않고 즉시 실패로 끝낸다.
	PasswordRequired,
};

// 서버가 보낸 Hello(op=0)의 내용.
struct FOBSHelloMessage
{
	FString ObsWebSocketVersion;
	int32   RpcVersion = 0;

	// authentication 필드가 있으면 true. 인증이 꺼진 서버는 이 필드 자체를 보내지 않는다.
	bool    bAuthenticationRequired = false;
	FString Challenge;
	FString Salt;
};

// 요청 응답(op=7)의 내용.
struct FOBSRequestResponse
{
	FString RequestType;
	FString RequestId;

	// requestStatus.result. 이 값이 곧 성공 여부이며 추측할 여지가 없다.
	bool    bSuccess = false;
	int32   Code = 0;
	FString Comment;

	// responseData. 없을 수 있으므로 항상 유효성을 확인하고 쓴다.
	TSharedPtr<FJsonObject> ResponseData;
};

// GetRecordStatus 응답 내용. 소유권 판정의 근거가 된다.
struct FOBSRecordStatus
{
	// true면 남의 녹화가 이미 돌고 있다는 뜻이다.
	bool  bOutputActive = false;
	bool  bOutputPaused = false;
	int64 OutputDurationMs = 0;
};

// RecordStateChanged 이벤트 내용. 요청 응답을 보조하는 용도로만 쓴다.
struct FOBSRecordStateChanged
{
	bool    bOutputActive = false;
	FString OutputState;
	FString OutputPath;
};

// obs-websocket 5.x 메시지의 조립·파싱과 인증 계산만 담당하는 정적 유틸리티.
// WebSocket 객체를 소유하지 않으므로 네트워크 없이 단위 테스트할 수 있다.
class FOBSWebSocketProtocol
{
public:
	// 수신 문자열에서 op와 d를 꺼낸다. 파싱 실패나 필드 누락이면 false를 돌려주고 크래시하지 않는다.
	static bool ParseEnvelope(const FString& Message, int32& OutOpCode, TSharedPtr<FJsonObject>& OutData);

	// Hello(op=0)의 d를 해석한다. authentication 필드 유무로 인증 필요 여부가 결정된다.
	static bool ParseHello(const TSharedPtr<FJsonObject>& Data, FOBSHelloMessage& OutHello);

	// obs-websocket 5.x 인증 문자열을 계산한다.
	// secret = Base64(SHA256(password + salt)), authentication = Base64(SHA256(secret + challenge))
	static FString ComputeAuthentication(const FString& Password, const FString& Salt, const FString& Challenge);

	// Identify(op=1) 메시지를 만든다. 인증이 꺼진 서버면 authentication 필드를 아예 넣지 않는다.
	static EOBSIdentifyResult BuildIdentify(const FOBSHelloMessage& Hello, const FString& Password, int32 EventSubscriptions, FString& OutMessage);

	// 요청(op=6) 메시지를 만든다. requestData가 필요 없는 요청만 v1에서 쓴다.
	static FString BuildRequest(const FString& RequestType, const FString& RequestId);

	// 요청 응답(op=7)의 d를 해석한다.
	static bool ParseRequestResponse(const TSharedPtr<FJsonObject>& Data, FOBSRequestResponse& OutResponse);

	// GetRecordStatus 응답의 responseData를 해석한다.
	static bool ParseRecordStatus(const TSharedPtr<FJsonObject>& ResponseData, FOBSRecordStatus& OutStatus);

	// StopRecord 응답의 responseData에서 녹화 파일 경로를 꺼낸다. 이 값이 파일 처리의 유일한 근거다.
	static bool ParseStopRecordOutputPath(const TSharedPtr<FJsonObject>& ResponseData, FString& OutPath);

	// 이벤트(op=5)가 RecordStateChanged면 내용을 해석한다. 다른 이벤트면 false.
	static bool ParseRecordStateChanged(const TSharedPtr<FJsonObject>& Data, FOBSRecordStateChanged& OutEvent);

	// 새 requestId를 만든다. 응답 상관관계의 열쇠이므로 매번 새 값이어야 한다.
	static FString MakeRequestId();
};
