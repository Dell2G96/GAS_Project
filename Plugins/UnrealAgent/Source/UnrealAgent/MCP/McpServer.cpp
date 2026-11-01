
#include "McpServer.h"
#include "HttpServerModule.h"
#include "IHttpRouter.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(McpServer)

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(McpServerLog, Log, All);

//-----------------------------------------------------------------------------
// UEditorSubsystem 오버라이드
//-----------------------------------------------------------------------------

void UMcpServer::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	DiscoverTools();
	StartHttpServer();
	StartServer();
}

void UMcpServer::Deinitialize()
{
	StopServer();
	StopHttpServer();

	Super::Deinitialize();

}

//-----------------------------------------------------------------------------
// Agent Server 프로세스
//-----------------------------------------------------------------------------

void UMcpServer::KillExistingAgentServer() const
{
	int32 ReturnCode;
	FString StdOut, StdErr;
	
	FPlatformProcess::ExecProcess(
		TEXT("C:\\Windows\\System32\\taskkill.exe"), TEXT("/f /im UnrealAgent.Frontend.exe"),
		&ReturnCode, &StdOut, &StdErr);
}

void UMcpServer::RestartServer()
{
	StopServer();
	StartServer();
}

void UMcpServer::StartServer()
{
	// 이전 세션에서 남아있는 기존 프로세스를 종료한다
	KillExistingAgentServer();
	
	const FString ServerExePath = FPaths::ConvertRelativePathToFull(
		TEXT("D:/_Projects/_UnrealLLM/UnrealAgentServer_UE56/Build/Release/UnrealAgent.Frontend.exe"));	// UE 5.6용 서버 복사본
	
	if (!FPaths::FileExists(ServerExePath))
	{
		UE_LOG(McpServerLog, Error, TEXT("Agent Server 실행 파일을 찾을 수 없습니다: %s"), *ServerExePath);
		return;
	}
	
	// Agent Server가 프로젝트 밖(별도 빌드 경로)에서 실행되므로,
	// .unrealagent 설정 디렉토리를 찾을 수 있도록 프로젝트 경로를 인자로 전달한다
	// 끝의 슬래시를 제거한다 (백슬래시로 끝나면 커맨드라인 파싱 시 닫는 따옴표를 이스케이프함)
	FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FPaths::NormalizeDirectoryName(ProjectDir);
	const FString Params = FString::Printf(TEXT("--project-dir \"%s\""), *ProjectDir);

	AgentServerProcess = FPlatformProcess::CreateProc(
		*ServerExePath, *Params,
		true,				//bLaunchDetached
		true,				// bLaunchHideen
		true,				// bLaunchReallyHidden
		&AgentServerProcessId,
		0, nullptr, nullptr);
			
	if (AgentServerProcess.IsValid())
	{
		UE_LOG(McpServerLog, Log, TEXT("Agent Server를 시작했습니다 (PID: %d)"), AgentServerProcessId);
		AssignAgentServerToJobObject();
	}

	else
	{
		UE_LOG(McpServerLog, Error, TEXT("Agent Server를 시작할 수 없습니다: %s"), *ServerExePath);
	}

}

void UMcpServer::StopServer()
{
	if (!AgentServerProcess.IsValid())
	{
		return;
	}
	
	if (FPlatformProcess::IsProcRunning(AgentServerProcess))
	{
		FPlatformProcess::TerminateProc(AgentServerProcess, true);
		UE_LOG(McpServerLog, Log, TEXT("Agent Server를 종료했습니다 (PID: %d)"), AgentServerProcessId);
	}
	
	FPlatformProcess::CloseProc(AgentServerProcess);
	AgentServerProcessId = 0;

#if PLATFORM_WINDOWS
	if (AgentServerJobHandle)
	{
		::CloseHandle(static_cast<HANDLE>(AgentServerJobHandle));
		AgentServerJobHandle = nullptr;
	}
#endif
}

void UMcpServer::AssignAgentServerToJobObject()
{
#if PLATFORM_WINDOWS
	// 에디터가 Deinitialize()를 거치지 않고 죽어도(크래시, 강제종료 등)
	// Windows가 Agent Server를 자동으로 정리하도록 Job Object에 묶어둔다.
	// 이 Job 핸들을 쥐고 있는 마지막 프로세스(=에디터)가 종료되는 순간
	// KILL_ON_JOB_CLOSE 설정에 의해 연결된 자식 프로세스도 함께 종료된다.
	HANDLE JobHandle = ::CreateJobObjectW(nullptr, nullptr);
	if (!JobHandle)
	{
		UE_LOG(McpServerLog, Warning, TEXT("Job Object 생성 실패 (에러: %d)"), ::GetLastError());
		return;
	}

	JOBOBJECT_EXTENDED_LIMIT_INFORMATION LimitInfo;
	FMemory::Memzero(LimitInfo);
	LimitInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

	if (!::SetInformationJobObject(JobHandle, JobObjectExtendedLimitInformation, &LimitInfo, sizeof(LimitInfo)))
	{
		UE_LOG(McpServerLog, Warning, TEXT("SetInformationJobObject 실패 (에러: %d)"), ::GetLastError());
		::CloseHandle(JobHandle);
		return;
	}

	if (!::AssignProcessToJobObject(JobHandle, AgentServerProcess.Get()))
	{
		UE_LOG(McpServerLog, Warning, TEXT("AssignProcessToJobObject 실패 (에러: %d)"), ::GetLastError());
		::CloseHandle(JobHandle);
		return;
	}

	AgentServerJobHandle = JobHandle;
#endif
}

//-----------------------------------------------------------------------------
// HTTP 서버
//-----------------------------------------------------------------------------
void UMcpServer::StartHttpServer()
{
	FHttpServerModule& HttpServerModule = FHttpServerModule::Get();
	HttpRouter = HttpServerModule.GetHttpRouter(ServerPort);
	
	if (!HttpRouter.IsValid())
	{
		UE_LOG(McpServerLog, Error, TEXT("HTTP 라우터를 생성할 수 없습니다 (포트: %d)"), ServerPort);

		return;
	}
	
	McpRouteHandle = HttpRouter->BindRoute(
		FHttpPath(TEXT("/mcp")),
		EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateUObject(this, &ThisClass::HandleMcpRequest));
	
	HttpServerModule.StartAllListeners();
}

void UMcpServer::StopHttpServer()
{
	if (HttpRouter.IsValid())
	{
		HttpRouter->UnbindRoute(McpRouteHandle);
		HttpRouter.Reset();
	}
}


//-----------------------------------------------------------------------------
// JSON-RPC 2.0 라우팅
//-----------------------------------------------------------------------------

bool UMcpServer::HandleMcpRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	// 요청 본문을 JSON으로 파싱
	const FUTF8ToTCHAR BodyConverter(reinterpret_cast<const char*>(Request.Body.GetData()), Request.Body.Num());
	const FString BodyString(BodyConverter.Length(), BodyConverter.Get());

	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(BodyString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		const FString ErrorJson = SerializeJson(MakeJsonRpcError(0, -32700, TEXT("Parse error")));
		OnComplete(FHttpServerResponse::Create(ErrorJson, TEXT("application/json")));

		return true;
	}

	// JSON-RPC 2.0 필드를 추출합니다
	const int32 RequestId = static_cast<int32>(JsonObject->GetNumberField(TEXT("id")));
	const FString Method = JsonObject->GetStringField(TEXT("method"));
	
	// 메서드별로 라우팅합니다
	TSharedPtr<FJsonObject> Response;

	if (Method == TEXT("initialize"))
	{
		Response = HandleInitialize(RequestId);
	}
	else if (Method == TEXT("tools/list"))
	{
		Response = HandleToolsList(RequestId);
	}
	else if (Method == TEXT("tools/call"))
	{
		const TSharedPtr<FJsonObject>* Params = nullptr;
		JsonObject->TryGetObjectField(TEXT("params"), Params);

		Response = HandleToolsCall(RequestId, Params ? *Params : nullptr);
	}
	else
	{
		Response = MakeJsonRpcError(RequestId, -32601, FString::Printf(TEXT("Method not found: %s"), *Method));
	}

	OnComplete(FHttpServerResponse::Create(SerializeJson(Response), TEXT("application/json")));

	return true;



}

TSharedPtr<FJsonObject> UMcpServer::HandleInitialize(int32 RequestId) const
{
	// 서버 정보를 구성
	const TSharedPtr<FJsonObject> ServerInfo = MakeShared<FJsonObject>();
	ServerInfo->SetStringField(TEXT("name"), TEXT("unreal-agent"));
	ServerInfo->SetStringField(TEXT("version"), TEXT("1.0.0"));
	
	// capailities : 도구 제공
	const TSharedPtr<FJsonObject> Tools = MakeShared<FJsonObject>(); 
	const TSharedPtr<FJsonObject> Capabilities = MakeShared<FJsonObject>();
	Capabilities->SetObjectField(TEXT("tools"), Tools);
	
	// result 조립
	const TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("protocolVersion"), TEXT("2025-03-26"));
	Result->SetObjectField(TEXT("serverInfo"),ServerInfo);
	Result->SetObjectField(TEXT("capabilities"), Capabilities);
	
	return MakeJsonRpcResponse(RequestId, Result);
}

TSharedPtr<FJsonObject> UMcpServer::HandleToolsList(int32 RequestId) const
{
	
	// 사전 빌드도니 도구 정의 목록을 반환
	const TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetArrayField(TEXT("tools"), ToolDefinitions);
	
	return MakeJsonRpcResponse(RequestId, Result);
}

TSharedPtr<FJsonObject> UMcpServer::HandleToolsCall(int32 RequestId, const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return MakeJsonRpcError(RequestId, -32602, TEXT("Missing params"));
	}
	
	const FString ToolName = Params->GetStringField(TEXT("name"));
	if (ToolName.IsEmpty())
	{
		return MakeJsonRpcError(RequestId, -32602, TEXT("Missing tool name"));
	}
	
	// arguments 호출 (없으면 빈 객체)
	const TSharedPtr<FJsonObject>* ArgumentsPtr = nullptr;
	Params->TryGetObjectField(TEXT("arguments"), ArgumentsPtr);
	const TSharedPtr<FJsonObject> Arguments = ArgumentsPtr ? *ArgumentsPtr : MakeShared<FJsonObject>();
	
	// 도구 실행
	const FMcpResponse ToolResponse = ExecuteTool(ToolName, Arguments);
	
	// MCP ToolCallResult 형식으로 변환
	const TSharedPtr<FJsonObject> ContentItem = MakeShared<FJsonObject>();
	ContentItem->SetStringField(TEXT("type"), TEXT("text"));
	ContentItem->SetStringField(TEXT("text"), ToolResponse.GetText());
	
	TArray<TSharedPtr<FJsonValue>> ContentArray;
	ContentArray.Add(MakeShared<FJsonValueObject>(ContentItem));
	
	const TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetArrayField(TEXT("content"), ContentArray);
	Result->SetBoolField(TEXT("isError"), !ToolResponse.bSuccess);
	
	return MakeJsonRpcResponse(RequestId, Result);
}

//-----------------------------------------------------------------------------
// 도구 관리
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
// JSON-RPC 유틸리티
//-----------------------------------------------------------------------------
TSharedPtr<FJsonObject> UMcpServer::MakeJsonRpcResponse(int32 Id, const TSharedPtr<FJsonObject>& Result)
{
	const TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
	Response->SetNumberField(TEXT("id"), Id);
	Response->SetObjectField(TEXT("result"), Result);

	return Response;
	
}

TSharedPtr<FJsonObject> UMcpServer::MakeJsonRpcError(int32 Id, int32 Code, const FString& Message)
{
	const TSharedPtr<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
	ErrorObj->SetNumberField(TEXT("code"), Code);
	ErrorObj->SetStringField(TEXT("message"), Message);

	const TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
	Response->SetNumberField(TEXT("id"), Id);
	Response->SetObjectField(TEXT("error"), ErrorObj);

	return Response;
}

FString UMcpServer::SerializeJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FString OutputString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString, 0);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(),Writer);
	
	return OutputString;
}

//-----------------------------------------------------------------------------
// 도구 관리
//-----------------------------------------------------------------------------
void UMcpServer::DiscoverTools()
{
	const UScriptStruct* BaseStruct = FMcpTool::StaticStruct();
	
	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		UScriptStruct* Struct = *It;
		
		// HIdden 메타가 있는 구조체는 제외
		if (Struct->HasMetaData(TEXT("Hidden")))
			continue;
		
		// FMcpTool을 상속하지 않으면 제외
		if (!Struct->IsChildOf(BaseStruct))
			continue;
		
		// MCPTool 메타에서 도구 이름을 읽는다
		const FString ToolName = Struct->GetMetaData(TEXT("McpTool"));
		if (ToolName.IsEmpty())
			continue;
		
		if (ToolMap.Contains(ToolName))
		{
			UE_LOG(McpServerLog, Error, TEXT("중복된 도구 이름: %s (%s, %s)"),
			*ToolName, *ToolMap[ToolName]->GetName(), *Struct->GetName());

			continue;
		}
		ToolMap.Add(ToolName, Struct);
		UE_LOG(McpServerLog, Log, TEXT("도구 등록됨: %s"), *ToolName);

		// MCp 도구 정의를 생성 
		const TSharedPtr<FJsonObject> ToolDef = MakeShared<FJsonObject>();
		ToolDef->SetStringField(TEXT("name"), ToolName);
		
		// 임시 인스턴스를 생성하여 ToolEscription()을 호출
		{
			uint8* StructMemory = static_cast<uint8*>(FMemory::Malloc
				(Struct->GetStructureSize()));
			Struct->InitializeStruct(StructMemory);
			
			const FMcpTool* ToolInstance = reinterpret_cast<const FMcpTool*>(StructMemory);
			const FString Description = ToolInstance->ToolDescription();
			
			Struct->DestroyStruct(StructMemory);
			FMemory::Free(StructMemory);
			
			if (!Description.IsEmpty())
				ToolDef->SetStringField(TEXT("description"), Description);
		}
		
		ToolDef->SetObjectField(TEXT("inputSchema"), BuildInputSchema(Struct));
		ToolDefinitions.Add(MakeShared<FJsonValueObject>(ToolDef));
	}

	UE_LOG(McpServerLog, Log, TEXT("도구 탐색 완료: 총 %d개"), ToolDefinitions.Num());
}

TSharedPtr<FJsonObject> UMcpServer::BuildInputSchema(const UScriptStruct* Struct) const
{
	const TSharedPtr<FJsonObject> Schema = MakeShared<FJsonObject>();
	Schema->SetStringField(TEXT("type"), TEXT("object"));
	
	const TSharedPtr<FJsonObject> Properties = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Required;
	
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FProperty* Property = *It;
		
		if (!Property->HasMetaData(TEXT("ToolParam")))
			continue;
		const FString JsonKey = GetParamJsonKey(Property);

		// UPROPERTY 타입 → JSON Schema 타입
		const TSharedPtr<FJsonObject> PropSchema = MakeShared<FJsonObject>();

		if (CastField<FStrProperty>(Property))
		{
			PropSchema->SetStringField(TEXT("type"), TEXT("string"));
		}
		else if (CastField<FIntProperty>(Property) || CastField<FInt64Property>(Property))
		{
			PropSchema->SetStringField(TEXT("type"), TEXT("integer"));
		}
		else if (CastField<FFloatProperty>(Property) || CastField<FDoubleProperty>(Property))
		{
			PropSchema->SetStringField(TEXT("type"), TEXT("number"));
		}
		else if (CastField<FBoolProperty>(Property))
		{
			PropSchema->SetStringField(TEXT("type"), TEXT("boolean"));
		}
		else
		{
			// 미지원 타입은 any로 처리합니다
			UE_LOG(McpServerLog, Warning, TEXT("미지원 ToolParam 타입: %s.%s (%s)"),
				*Struct->GetName(), *Property->GetName(), *Property->GetCPPType());
		}

		// Description 메타 → JSON Schema description
		if (Property->HasMetaData(TEXT("Description")))
		{
			PropSchema->SetStringField(TEXT("description"), Property->GetMetaData(TEXT("Description")));
		}

		Properties->SetObjectField(JsonKey, PropSchema);

		// Required 메타 → required 배열
		if (Property->HasMetaData(TEXT("Required")))
		{
			Required.Add(MakeShared<FJsonValueString>(JsonKey));
		}
	}

	Schema->SetObjectField(TEXT("properties"), Properties);

	if (Required.Num() > 0)
	{
		Schema->SetArrayField(TEXT("required"), Required);
	}

	return Schema;
}

void UMcpServer::PopulateToolParams(const UScriptStruct* Struct, void* ToolMemory,
	const TSharedPtr<FJsonObject>& Arguments) const
{
	if (!Arguments.IsValid())
	{
		return;
	}

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		FProperty* Property = *It;

		if (!Property->HasMetaData(TEXT("ToolParam")))
		{
			continue;
		}

		const FString JsonKey = GetParamJsonKey(Property);

		if (!Arguments->HasField(JsonKey))
		{
			continue;
		}

		if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
		{
			StrProp->SetPropertyValue_InContainer(ToolMemory, Arguments->GetStringField(JsonKey));
		}
		else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
		{
			IntProp->SetPropertyValue_InContainer(ToolMemory, static_cast<int32>(Arguments->GetNumberField(JsonKey)));
		}
		else if (FInt64Property* Int64Prop = CastField<FInt64Property>(Property))
		{
			Int64Prop->SetPropertyValue_InContainer(ToolMemory, static_cast<int64>(Arguments->GetNumberField(JsonKey)));
		}
		else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
		{
			FloatProp->SetPropertyValue_InContainer(ToolMemory, static_cast<float>(Arguments->GetNumberField(JsonKey)));
		}
		else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
		{
			DoubleProp->SetPropertyValue_InContainer(ToolMemory, Arguments->GetNumberField(JsonKey));
		}
		else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			BoolProp->SetPropertyValue_InContainer(ToolMemory, Arguments->GetBoolField(JsonKey));
		}
	}
}


FString UMcpServer::GetParamJsonKey(const FProperty* Property)
{
	const FString MetaValue = Property->GetMetaData(TEXT("ToolParam"));
	
	// ToolParam="custom_key" 형태면 해당 값을 리턴, 아니면 프로퍼티 이름을 사용한다
	if (!MetaValue.IsEmpty() && MetaValue != TEXT("TRUE"))
		return MetaValue;
	
	return Property->GetName();
}

FMcpResponse UMcpServer::ExecuteTool(const FString& Name, const TSharedPtr<FJsonObject>& Arguments)
{
	// 도구를 찾습니다
	UScriptStruct** FoundStruct = ToolMap.Find(Name);
	if (!FoundStruct)
	{
		return FMcpResponse::Failure(FString::Printf(TEXT("Unknown tool: %s"), *Name));
	}

	UScriptStruct* Struct = *FoundStruct;

	// 도구 구조체를 힙에 할당하고 초기화합니다
	void* Memory = FMemory::Malloc(Struct->GetStructureSize(), Struct->GetMinAlignment());
	Struct->InitializeStruct(Memory);

	FMcpTool* Tool = static_cast<FMcpTool*>(Memory);

	// ToolParam UPROPERTY에 arguments를 역직렬화합니다
	Tool->Args = Arguments;
	PopulateToolParams(Struct, Memory, Arguments);

	FMcpResponse Response = Tool->Execute();

	// 구조체를 정리합니다
	Struct->DestroyStruct(Memory);
	FMemory::Free(Memory);

	return Response;
}




