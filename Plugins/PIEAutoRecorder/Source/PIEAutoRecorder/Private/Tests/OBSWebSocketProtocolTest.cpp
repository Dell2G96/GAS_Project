#include "Misc/AutomationTest.h"

#include "OBS/OBSSha256.h"
#include "OBS/OBSWebSocketProtocol.h"
#include "Dom/JsonObject.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PIEAutoRecorderTest
{
	// 테스트에서 반복해 쓰는 값. Salt/Challenge는 obs-websocket이 주는 형태(Base64)를 흉내 낸다.
	const FString Password = TEXT("supersecretpassword");
	const FString Salt = TEXT("lM1GncleQOaCu9lT1yeUZhFYnqhsLLP1G5lAGo3ixaI=");
	const FString Challenge = TEXT("+IxH4CnCiqpX1rM9scsNynZzbOe4KhDeYcTNS3PDaeY=");

	// 위 세 값으로 계산되어야 하는 기대 결과. .NET 암호화 라이브러리로 독립 산출했다.
	const FString ExpectedSecret = TEXT("H1IfVz1pSREUQzbFTVnX/Tyb+gMhMik5x7yUBCY0PTs=");
	const FString ExpectedAuth = TEXT("1Ct943GAT+6YQUUX47Ia/ncufilbe6+oD6lY+5kaCu4=");
}

//========================================================================================
// 1. SHA-256 자체가 옳은가 — NIST 표준 테스트 벡터
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSSha256Test, "PIEAutoRecorder.OBS.Sha256",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// OpenSSL 연결과 UTF-8 변환이 올바른지 표준 벡터로 확인한다.
bool FOBSSha256Test::RunTest(const FString& Parameters)
{
	// NIST 표준 값. 이 두 개가 맞으면 해시 함수 자체는 신뢰할 수 있다.
	TestEqual(TEXT("SHA256(\"abc\")"),
		OBSSha256::HashUtf8ToHex(TEXT("abc")),
		TEXT("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));

	TestEqual(TEXT("SHA256(빈 문자열)"),
		OBSSha256::HashUtf8ToHex(TEXT("")),
		TEXT("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));

	// Base64 인코딩 경로도 함께 확인한다.
	TestEqual(TEXT("Base64(SHA256(\"abc\"))"),
		OBSSha256::HashUtf8ToBase64(TEXT("abc")),
		TEXT("ungWv48Bz+pBQUDeXa4iI7ADYaOWF3qctBD/YfIAFa0="));

	return true;
}

//========================================================================================
// 2. 인증 계산 — 2단계 해시가 규약대로인가
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSAuthenticationTest, "PIEAutoRecorder.OBS.Authentication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// secret과 authentication 두 단계를 각각 기대값과 대조한다.
bool FOBSAuthenticationTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderTest;

	// 1단계: secret = Base64(SHA256(password + salt))
	TestEqual(TEXT("secret 계산"),
		OBSSha256::HashUtf8ToBase64(Password + Salt),
		ExpectedSecret);

	// 2단계: authentication = Base64(SHA256(secret + challenge))
	TestEqual(TEXT("authentication 계산"),
		FOBSWebSocketProtocol::ComputeAuthentication(Password, Salt, Challenge),
		ExpectedAuth);

	// UTF-8 변환이 실제로 일어나는지 확인한다. TCHAR 그대로 해시하면 이 값이 달라진다.
	TestEqual(TEXT("한글 비밀번호 UTF-8 처리"),
		FOBSWebSocketProtocol::ComputeAuthentication(TEXT("한글비밀번호"), TEXT("c2FsdA=="), TEXT("Y2hhbGxlbmdl")),
		TEXT("J2wAb7nR7YtW1aNaXcBeXaVbqdXJ/UdjbkAoVhchv10="));

	// salt나 challenge가 한 글자만 달라도 결과가 완전히 달라져야 한다.
	TestNotEqual(TEXT("challenge가 다르면 결과도 다름"),
		FOBSWebSocketProtocol::ComputeAuthentication(Password, Salt, Challenge),
		FOBSWebSocketProtocol::ComputeAuthentication(Password, Salt, Challenge + TEXT("x")));

	return true;
}

//========================================================================================
// 3. Hello 파싱 — 인증 있는 서버와 없는 서버를 구분하는가
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSHelloParseTest, "PIEAutoRecorder.OBS.HelloParse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 인증 on/off 두 형태의 Hello를 모두 정확히 구분하는지 확인한다.
bool FOBSHelloParseTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderTest;

	// 인증이 켜진 서버의 Hello
	{
		const FString Message = FString::Printf(
			TEXT("{\"op\":0,\"d\":{\"obsWebSocketVersion\":\"5.5.4\",\"rpcVersion\":1,")
			TEXT("\"authentication\":{\"challenge\":\"%s\",\"salt\":\"%s\"}}}"),
			*Challenge, *Salt);

		int32 OpCode = -1;
		TSharedPtr<FJsonObject> Data;
		TestTrue(TEXT("봉투 파싱 성공"), FOBSWebSocketProtocol::ParseEnvelope(Message, OpCode, Data));
		TestEqual(TEXT("op = 0"), OpCode, static_cast<int32>(EOBSOpCode::Hello));

		FOBSHelloMessage Hello;
		TestTrue(TEXT("Hello 파싱 성공"), FOBSWebSocketProtocol::ParseHello(Data, Hello));
		TestTrue(TEXT("인증 필요로 판정"), Hello.bAuthenticationRequired);
		TestEqual(TEXT("salt 추출"), Hello.Salt, Salt);
		TestEqual(TEXT("challenge 추출"), Hello.Challenge, Challenge);
		TestEqual(TEXT("rpcVersion 추출"), Hello.RpcVersion, 1);
		TestEqual(TEXT("버전 추출"), Hello.ObsWebSocketVersion, TEXT("5.5.4"));
	}

	// 인증이 꺼진 서버의 Hello — authentication 필드 자체가 없다
	{
		const FString Message = TEXT("{\"op\":0,\"d\":{\"obsWebSocketVersion\":\"5.5.4\",\"rpcVersion\":1}}");

		int32 OpCode = -1;
		TSharedPtr<FJsonObject> Data;
		TestTrue(TEXT("봉투 파싱 성공"), FOBSWebSocketProtocol::ParseEnvelope(Message, OpCode, Data));

		FOBSHelloMessage Hello;
		TestTrue(TEXT("Hello 파싱 성공"), FOBSWebSocketProtocol::ParseHello(Data, Hello));
		TestFalse(TEXT("인증 불필요로 판정"), Hello.bAuthenticationRequired);
		TestTrue(TEXT("salt 비어 있음"), Hello.Salt.IsEmpty());
	}

	// authentication은 있는데 salt가 빠진 비정상 Hello — 크래시 없이 인증 불필요로 처리
	{
		const FString Message = TEXT("{\"op\":0,\"d\":{\"rpcVersion\":1,\"authentication\":{\"challenge\":\"abc\"}}}");

		int32 OpCode = -1;
		TSharedPtr<FJsonObject> Data;
		FOBSWebSocketProtocol::ParseEnvelope(Message, OpCode, Data);

		FOBSHelloMessage Hello;
		AddExpectedError(TEXT("challenge 또는 salt가 없습니다"), EAutomationExpectedErrorFlags::Contains, 1);
		TestTrue(TEXT("Hello 파싱은 성공"), FOBSWebSocketProtocol::ParseHello(Data, Hello));
		TestFalse(TEXT("불완전한 authentication은 인증 불필요 처리"), Hello.bAuthenticationRequired);
	}

	return true;
}

//========================================================================================
// 4. Identify 조립 — 인증 필드를 넣고 빼는 분기가 옳은가
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSIdentifyBuildTest, "PIEAutoRecorder.OBS.Identify",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 인증 on/off, 비밀번호 누락 세 경우의 Identify 조립 결과를 확인한다.
bool FOBSIdentifyBuildTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderTest;

	const int32 OutputsSubscription = static_cast<int32>(EOBSEventSubscription::Outputs);
	TestEqual(TEXT("Outputs 구독 비트 = 64"), OutputsSubscription, 64);

	// 인증이 필요한 경우 — authentication 필드가 들어가야 한다
	{
		FOBSHelloMessage Hello;
		Hello.RpcVersion = 1;
		Hello.bAuthenticationRequired = true;
		Hello.Salt = Salt;
		Hello.Challenge = Challenge;

		FString Message;
		TestEqual(TEXT("조립 성공"),
			FOBSWebSocketProtocol::BuildIdentify(Hello, Password, OutputsSubscription, Message),
			EOBSIdentifyResult::Success);

		TestTrue(TEXT("op=1 포함"), Message.Contains(TEXT("\"op\":1")));
		TestTrue(TEXT("rpcVersion 포함"), Message.Contains(TEXT("\"rpcVersion\":1")));
		TestTrue(TEXT("eventSubscriptions 포함"), Message.Contains(TEXT("\"eventSubscriptions\":64")));
		TestTrue(TEXT("계산된 authentication 포함"), Message.Contains(ExpectedAuth));
	}

	// 인증이 필요 없는 경우 — authentication 필드가 아예 없어야 한다
	{
		FOBSHelloMessage Hello;
		Hello.RpcVersion = 1;
		Hello.bAuthenticationRequired = false;

		FString Message;
		TestEqual(TEXT("조립 성공"),
			FOBSWebSocketProtocol::BuildIdentify(Hello, TEXT(""), OutputsSubscription, Message),
			EOBSIdentifyResult::Success);

		TestFalse(TEXT("authentication 필드 없음"), Message.Contains(TEXT("authentication")));
	}

	// 인증이 필요한데 비밀번호가 빈 경우 — 계산을 시도하지 않고 실패로 끝난다
	{
		FOBSHelloMessage Hello;
		Hello.RpcVersion = 1;
		Hello.bAuthenticationRequired = true;
		Hello.Salt = Salt;
		Hello.Challenge = Challenge;

		FString Message;
		TestEqual(TEXT("비밀번호 필요 판정"),
			FOBSWebSocketProtocol::BuildIdentify(Hello, TEXT(""), OutputsSubscription, Message),
			EOBSIdentifyResult::PasswordRequired);

		TestTrue(TEXT("메시지를 만들지 않음"), Message.IsEmpty());
	}

	return true;
}

//========================================================================================
// 5. 요청 조립과 응답 파싱
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSRequestResponseTest, "PIEAutoRecorder.OBS.RequestResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 요청 조립, 성공·실패 응답, GetRecordStatus, StopRecord 경로 추출을 확인한다.
bool FOBSRequestResponseTest::RunTest(const FString& Parameters)
{
	// 요청 조립
	{
		const FString RequestId = TEXT("11111111-2222-3333-4444-555555555555");
		const FString Message = FOBSWebSocketProtocol::BuildRequest(TEXT("StartRecord"), RequestId);

		TestTrue(TEXT("op=6 포함"), Message.Contains(TEXT("\"op\":6")));
		TestTrue(TEXT("requestType 포함"), Message.Contains(TEXT("\"requestType\":\"StartRecord\"")));
		TestTrue(TEXT("requestId 포함"), Message.Contains(RequestId));
	}

	// requestId는 매번 달라야 한다
	{
		TestNotEqual(TEXT("requestId가 매번 새로 생성됨"),
			FOBSWebSocketProtocol::MakeRequestId(),
			FOBSWebSocketProtocol::MakeRequestId());
	}

	// 성공 응답 + StopRecord의 outputPath
	{
		const FString Message =
			TEXT("{\"op\":7,\"d\":{\"requestType\":\"StopRecord\",\"requestId\":\"REQ-1\",")
			TEXT("\"requestStatus\":{\"result\":true,\"code\":100},")
			TEXT("\"responseData\":{\"outputPath\":\"D:/Recordings/test.mkv\"}}}");

		int32 OpCode = -1;
		TSharedPtr<FJsonObject> Data;
		TestTrue(TEXT("봉투 파싱 성공"), FOBSWebSocketProtocol::ParseEnvelope(Message, OpCode, Data));
		TestEqual(TEXT("op = 7"), OpCode, static_cast<int32>(EOBSOpCode::RequestResponse));

		FOBSRequestResponse Response;
		TestTrue(TEXT("응답 파싱 성공"), FOBSWebSocketProtocol::ParseRequestResponse(Data, Response));
		TestTrue(TEXT("성공으로 판정"), Response.bSuccess);
		TestEqual(TEXT("code 추출"), Response.Code, 100);
		TestEqual(TEXT("requestId 추출"), Response.RequestId, TEXT("REQ-1"));

		FString OutputPath;
		TestTrue(TEXT("outputPath 추출"), FOBSWebSocketProtocol::ParseStopRecordOutputPath(Response.ResponseData, OutputPath));
		TestEqual(TEXT("경로 값"), OutputPath, TEXT("D:/Recordings/test.mkv"));
	}

	// 실패 응답 — comment까지 살려야 원인을 로그에 남길 수 있다
	{
		const FString Message =
			TEXT("{\"op\":7,\"d\":{\"requestType\":\"StartRecord\",\"requestId\":\"REQ-2\",")
			TEXT("\"requestStatus\":{\"result\":false,\"code\":500,\"comment\":\"Output already active\"}}}");

		int32 OpCode = -1;
		TSharedPtr<FJsonObject> Data;
		FOBSWebSocketProtocol::ParseEnvelope(Message, OpCode, Data);

		FOBSRequestResponse Response;
		TestTrue(TEXT("응답 파싱 성공"), FOBSWebSocketProtocol::ParseRequestResponse(Data, Response));
		TestFalse(TEXT("실패로 판정"), Response.bSuccess);
		TestEqual(TEXT("comment 추출"), Response.Comment, TEXT("Output already active"));
		TestFalse(TEXT("responseData 없음"), Response.ResponseData.IsValid());
	}

	// GetRecordStatus — 남의 녹화가 돌고 있는 경우
	{
		const FString Message =
			TEXT("{\"op\":7,\"d\":{\"requestType\":\"GetRecordStatus\",\"requestId\":\"REQ-3\",")
			TEXT("\"requestStatus\":{\"result\":true,\"code\":100},")
			TEXT("\"responseData\":{\"outputActive\":true,\"outputPaused\":false,\"outputDuration\":12345}}}");

		int32 OpCode = -1;
		TSharedPtr<FJsonObject> Data;
		FOBSWebSocketProtocol::ParseEnvelope(Message, OpCode, Data);

		FOBSRequestResponse Response;
		FOBSWebSocketProtocol::ParseRequestResponse(Data, Response);

		FOBSRecordStatus Status;
		TestTrue(TEXT("상태 파싱 성공"), FOBSWebSocketProtocol::ParseRecordStatus(Response.ResponseData, Status));
		TestTrue(TEXT("outputActive=true"), Status.bOutputActive);
		TestEqual(TEXT("outputDuration 추출"), Status.OutputDurationMs, static_cast<int64>(12345));
	}

	// RecordStateChanged 이벤트
	{
		const FString Message =
			TEXT("{\"op\":5,\"d\":{\"eventType\":\"RecordStateChanged\",")
			TEXT("\"eventData\":{\"outputActive\":false,\"outputState\":\"OBS_WEBSOCKET_OUTPUT_STOPPED\",")
			TEXT("\"outputPath\":\"D:/Recordings/a.mkv\"}}}");

		int32 OpCode = -1;
		TSharedPtr<FJsonObject> Data;
		FOBSWebSocketProtocol::ParseEnvelope(Message, OpCode, Data);
		TestEqual(TEXT("op = 5"), OpCode, static_cast<int32>(EOBSOpCode::Event));

		FOBSRecordStateChanged Event;
		TestTrue(TEXT("이벤트 파싱 성공"), FOBSWebSocketProtocol::ParseRecordStateChanged(Data, Event));
		TestFalse(TEXT("outputActive=false"), Event.bOutputActive);
		TestEqual(TEXT("outputPath 추출"), Event.OutputPath, TEXT("D:/Recordings/a.mkv"));
	}

	// 관심 없는 다른 이벤트는 false로 걸러진다
	{
		const FString Message = TEXT("{\"op\":5,\"d\":{\"eventType\":\"CurrentProgramSceneChanged\",\"eventData\":{}}}");

		int32 OpCode = -1;
		TSharedPtr<FJsonObject> Data;
		FOBSWebSocketProtocol::ParseEnvelope(Message, OpCode, Data);

		FOBSRecordStateChanged Event;
		TestFalse(TEXT("다른 이벤트는 무시"), FOBSWebSocketProtocol::ParseRecordStateChanged(Data, Event));
	}

	return true;
}

//========================================================================================
// 6. 깨진 입력 — 크래시하지 않고 false로 끝나는가
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSMalformedInputTest, "PIEAutoRecorder.OBS.MalformedInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 어떤 쓰레기 입력이 와도 에디터가 죽지 않아야 한다.
bool FOBSMalformedInputTest::RunTest(const FString& Parameters)
{
	const TArray<FString> BadMessages =
	{
		TEXT(""),											// 빈 문자열
		TEXT("not json at all"),							// JSON이 아님
		TEXT("{"),											// 잘린 JSON
		TEXT("{\"op\":7}"),									// d 없음
		TEXT("{\"d\":{}}"),									// op 없음
		TEXT("[1,2,3]"),									// 객체가 아님
		TEXT("{\"op\":\"seven\",\"d\":{}}"),				// op 타입 오류
	};

	// 개수를 세지 않고 어떤 경고든 허용한다. 여기서 확인할 것은 "죽지 않는다"이다.
	AddExpectedError(TEXT("OBS 메시지"), EAutomationExpectedErrorFlags::Contains, 0);

	for (const FString& Bad : BadMessages)
	{
		int32 OpCode = -1;
		TSharedPtr<FJsonObject> Data;
		FOBSWebSocketProtocol::ParseEnvelope(Bad, OpCode, Data);
	}

	// null 데이터로 각 파서를 호출해도 안전해야 한다
	{
		const TSharedPtr<FJsonObject> NullData;

		FOBSHelloMessage Hello;
		TestFalse(TEXT("null Hello"), FOBSWebSocketProtocol::ParseHello(NullData, Hello));

		FOBSRequestResponse Response;
		TestFalse(TEXT("null 응답"), FOBSWebSocketProtocol::ParseRequestResponse(NullData, Response));

		FOBSRecordStatus Status;
		TestFalse(TEXT("null 상태"), FOBSWebSocketProtocol::ParseRecordStatus(NullData, Status));

		FString Path;
		TestFalse(TEXT("null 경로"), FOBSWebSocketProtocol::ParseStopRecordOutputPath(NullData, Path));

		FOBSRecordStateChanged Event;
		TestFalse(TEXT("null 이벤트"), FOBSWebSocketProtocol::ParseRecordStateChanged(NullData, Event));
	}

	// requestId가 없는 응답은 처리 불가로 걸러진다
	{
		int32 OpCode = -1;
		TSharedPtr<FJsonObject> Data;
		FOBSWebSocketProtocol::ParseEnvelope(TEXT("{\"op\":7,\"d\":{\"requestType\":\"StartRecord\"}}"), OpCode, Data);

		AddExpectedError(TEXT("requestId가 없습니다"), EAutomationExpectedErrorFlags::Contains, 1);

		FOBSRequestResponse Response;
		TestFalse(TEXT("requestId 없는 응답은 거부"), FOBSWebSocketProtocol::ParseRequestResponse(Data, Response));
	}

	// outputActive가 없는 상태 응답은 추측하지 않고 실패 처리
	{
		int32 OpCode = -1;
		TSharedPtr<FJsonObject> Data;
		FOBSWebSocketProtocol::ParseEnvelope(
			TEXT("{\"op\":7,\"d\":{\"requestType\":\"GetRecordStatus\",\"requestId\":\"R\",")
			TEXT("\"requestStatus\":{\"result\":true},\"responseData\":{\"outputPaused\":false}}}"),
			OpCode, Data);

		FOBSRequestResponse Response;
		FOBSWebSocketProtocol::ParseRequestResponse(Data, Response);

		AddExpectedError(TEXT("outputActive가 없습니다"), EAutomationExpectedErrorFlags::Contains, 1);

		FOBSRecordStatus Status;
		TestFalse(TEXT("outputActive 없으면 실패"), FOBSWebSocketProtocol::ParseRecordStatus(Response.ResponseData, Status));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
