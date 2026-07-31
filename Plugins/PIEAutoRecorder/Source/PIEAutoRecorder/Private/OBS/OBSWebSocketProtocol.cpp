#include "OBSWebSocketProtocol.h"

#include "OBSSha256.h"
#include "PIEAutoRecorderLog.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	// 조립한 JSON을 한 줄 문자열로 만든다.
	FString SerializeObject(const TSharedRef<FJsonObject>& Object)
	{
		FString Output;
		const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Output);

		FJsonSerializer::Serialize(Object, Writer);
		return Output;
	}
}

// 수신 문자열을 JSON으로 읽고 op와 d를 꺼낸다. 어떤 입력이 와도 크래시하지 않고 false로 끝난다.
bool FOBSWebSocketProtocol::ParseEnvelope(const FString& Message, int32& OutOpCode, TSharedPtr<FJsonObject>& OutData)
{
	OutOpCode = -1;
	OutData.Reset();

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Message);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("OBS 메시지를 JSON으로 읽지 못했습니다. 무시합니다."));
		return false;
	}

	double OpValue = 0.0;
	if (!Root->TryGetNumberField(TEXT("op"), OpValue))
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("OBS 메시지에 op 필드가 없습니다. 무시합니다."));
		return false;
	}

	const TSharedPtr<FJsonObject>* DataObject = nullptr;
	if (!Root->TryGetObjectField(TEXT("d"), DataObject) || DataObject == nullptr || !DataObject->IsValid())
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("OBS 메시지에 d 필드가 없습니다. 무시합니다. (op=%d)"), static_cast<int32>(OpValue));
		return false;
	}

	OutOpCode = static_cast<int32>(OpValue);
	OutData = *DataObject;
	return true;
}

// Hello의 d를 해석한다. authentication 필드가 있으면 인증이 필요한 서버다.
bool FOBSWebSocketProtocol::ParseHello(const TSharedPtr<FJsonObject>& Data, FOBSHelloMessage& OutHello)
{
	if (!Data.IsValid())
	{
		return false;
	}

	OutHello = FOBSHelloMessage();
	Data->TryGetStringField(TEXT("obsWebSocketVersion"), OutHello.ObsWebSocketVersion);

	double RpcValue = 0.0;
	if (Data->TryGetNumberField(TEXT("rpcVersion"), RpcValue))
	{
		OutHello.RpcVersion = static_cast<int32>(RpcValue);
	}

	// 인증이 꺼진 서버는 이 필드 자체를 보내지 않는다. 이 분기 하나로 인증 on/off 양쪽이 처리된다.
	const TSharedPtr<FJsonObject>* AuthObject = nullptr;
	if (Data->TryGetObjectField(TEXT("authentication"), AuthObject) && AuthObject != nullptr && AuthObject->IsValid())
	{
		const bool bHasChallenge = (*AuthObject)->TryGetStringField(TEXT("challenge"), OutHello.Challenge);
		const bool bHasSalt = (*AuthObject)->TryGetStringField(TEXT("salt"), OutHello.Salt);

		// 두 값이 모두 있어야 계산이 가능하다. 하나라도 없으면 인증 불가로 보고 필요 없음 처리한다.
		OutHello.bAuthenticationRequired = bHasChallenge && bHasSalt;
		if (!OutHello.bAuthenticationRequired)
		{
			UE_LOG(LogPIEAutoRecorder, Warning, TEXT("Hello의 authentication에 challenge 또는 salt가 없습니다."));
			OutHello.Challenge.Reset();
			OutHello.Salt.Reset();
		}
	}

	return true;
}

// obs-websocket 5.x 공식 규약대로 인증 문자열을 2단계로 계산한다. 결과는 절대 로그로 남기지 않는다.
FString FOBSWebSocketProtocol::ComputeAuthentication(const FString& Password, const FString& Salt, const FString& Challenge)
{
	const FString Secret = OBSSha256::HashUtf8ToBase64(Password + Salt);
	return OBSSha256::HashUtf8ToBase64(Secret + Challenge);
}

// Identify 메시지를 조립한다. 인증이 필요한데 비밀번호가 비면 계산하지 않고 실패로 끝낸다.
EOBSIdentifyResult FOBSWebSocketProtocol::BuildIdentify(const FOBSHelloMessage& Hello, const FString& Password, int32 EventSubscriptions, FString& OutMessage)
{
	OutMessage.Reset();

	if (Hello.bAuthenticationRequired && Password.IsEmpty())
	{
		return EOBSIdentifyResult::PasswordRequired;
	}

	const TSharedRef<FJsonObject> DataObject = MakeShared<FJsonObject>();
	DataObject->SetNumberField(TEXT("rpcVersion"), Hello.RpcVersion > 0 ? Hello.RpcVersion : 1);
	DataObject->SetNumberField(TEXT("eventSubscriptions"), EventSubscriptions);

	if (Hello.bAuthenticationRequired)
	{
		DataObject->SetStringField(TEXT("authentication"), ComputeAuthentication(Password, Hello.Salt, Hello.Challenge));
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("op"), static_cast<int32>(EOBSOpCode::Identify));
	Root->SetObjectField(TEXT("d"), DataObject);

	OutMessage = SerializeObject(Root);
	return EOBSIdentifyResult::Success;
}

// 요청 메시지를 조립한다. v1이 쓰는 세 요청은 모두 추가 인자가 없다.
FString FOBSWebSocketProtocol::BuildRequest(const FString& RequestType, const FString& RequestId)
{
	const TSharedRef<FJsonObject> DataObject = MakeShared<FJsonObject>();
	DataObject->SetStringField(TEXT("requestType"), RequestType);
	DataObject->SetStringField(TEXT("requestId"), RequestId);

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("op"), static_cast<int32>(EOBSOpCode::Request));
	Root->SetObjectField(TEXT("d"), DataObject);

	return SerializeObject(Root);
}

// 요청 응답을 해석한다. requestStatus.result가 곧 성공 여부다.
bool FOBSWebSocketProtocol::ParseRequestResponse(const TSharedPtr<FJsonObject>& Data, FOBSRequestResponse& OutResponse)
{
	if (!Data.IsValid())
	{
		return false;
	}

	OutResponse = FOBSRequestResponse();
	Data->TryGetStringField(TEXT("requestType"), OutResponse.RequestType);

	// requestId가 없으면 어느 요청의 응답인지 알 수 없으므로 처리할 수 없다.
	if (!Data->TryGetStringField(TEXT("requestId"), OutResponse.RequestId) || OutResponse.RequestId.IsEmpty())
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("요청 응답에 requestId가 없습니다. 무시합니다."));
		return false;
	}

	const TSharedPtr<FJsonObject>* StatusObject = nullptr;
	if (!Data->TryGetObjectField(TEXT("requestStatus"), StatusObject) || StatusObject == nullptr || !StatusObject->IsValid())
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("요청 응답에 requestStatus가 없습니다. 실패로 처리합니다. (requestId=%s)"), *OutResponse.RequestId);
		return true;
	}

	(*StatusObject)->TryGetBoolField(TEXT("result"), OutResponse.bSuccess);
	(*StatusObject)->TryGetStringField(TEXT("comment"), OutResponse.Comment);

	double CodeValue = 0.0;
	if ((*StatusObject)->TryGetNumberField(TEXT("code"), CodeValue))
	{
		OutResponse.Code = static_cast<int32>(CodeValue);
	}

	// responseData는 없을 수 있다. StartRecord 성공 응답에는 들어 있지 않다.
	const TSharedPtr<FJsonObject>* ResponseDataObject = nullptr;
	if (Data->TryGetObjectField(TEXT("responseData"), ResponseDataObject) && ResponseDataObject != nullptr)
	{
		OutResponse.ResponseData = *ResponseDataObject;
	}

	return true;
}

// GetRecordStatus 응답을 해석한다. outputActive가 소유권 판정의 근거다.
bool FOBSWebSocketProtocol::ParseRecordStatus(const TSharedPtr<FJsonObject>& ResponseData, FOBSRecordStatus& OutStatus)
{
	if (!ResponseData.IsValid())
	{
		return false;
	}

	OutStatus = FOBSRecordStatus();

	// outputActive가 없으면 상태를 알 수 없다. 추측하지 않고 실패로 끝낸다.
	if (!ResponseData->TryGetBoolField(TEXT("outputActive"), OutStatus.bOutputActive))
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("GetRecordStatus 응답에 outputActive가 없습니다."));
		return false;
	}

	ResponseData->TryGetBoolField(TEXT("outputPaused"), OutStatus.bOutputPaused);

	double DurationValue = 0.0;
	if (ResponseData->TryGetNumberField(TEXT("outputDuration"), DurationValue))
	{
		OutStatus.OutputDurationMs = static_cast<int64>(DurationValue);
	}

	return true;
}

// StopRecord 응답에서 파일 경로를 꺼낸다. 비어 있으면 파일을 건드리지 않는다.
bool FOBSWebSocketProtocol::ParseStopRecordOutputPath(const TSharedPtr<FJsonObject>& ResponseData, FString& OutPath)
{
	OutPath.Reset();

	if (!ResponseData.IsValid())
	{
		return false;
	}

	return ResponseData->TryGetStringField(TEXT("outputPath"), OutPath) && !OutPath.IsEmpty();
}

// 이벤트가 RecordStateChanged인지 확인하고 내용을 해석한다.
bool FOBSWebSocketProtocol::ParseRecordStateChanged(const TSharedPtr<FJsonObject>& Data, FOBSRecordStateChanged& OutEvent)
{
	if (!Data.IsValid())
	{
		return false;
	}

	FString EventType;
	if (!Data->TryGetStringField(TEXT("eventType"), EventType) || EventType != TEXT("RecordStateChanged"))
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* EventDataObject = nullptr;
	if (!Data->TryGetObjectField(TEXT("eventData"), EventDataObject) || EventDataObject == nullptr || !EventDataObject->IsValid())
	{
		return false;
	}

	OutEvent = FOBSRecordStateChanged();
	(*EventDataObject)->TryGetBoolField(TEXT("outputActive"), OutEvent.bOutputActive);
	(*EventDataObject)->TryGetStringField(TEXT("outputState"), OutEvent.OutputState);
	(*EventDataObject)->TryGetStringField(TEXT("outputPath"), OutEvent.OutputPath);

	return true;
}

// 요청마다 새 requestId를 만든다. 늦게 온 응답을 구분하는 유일한 수단이다.
FString FOBSWebSocketProtocol::MakeRequestId()
{
	return FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
}
