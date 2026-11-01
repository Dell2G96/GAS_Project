#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Mcp/McpTypes.h"
#include "IHttpRouter.h"
#include "McpServer.generated.h"
/*
 * MCP 서버
 * 
 * 에디터 시작시 HTTP 서버 (포트 55559) 에서 MCP 프로토콜을 서빙한다
 * FMcpTool 파생 구조체를 리플렉션으로 자동 검색하여 등록하고
 * ToolParam UPROPERTY 로 부터 inputSchema 를 자동 생성한다
 * 
 * JSON-RPC 2.0 앤드포인트 : POST / mcp
 * - initialize : 서버 정보 및 capabilities 반환
 * - tools/list : 등록된 도구 목록 + inputschema 반환
 * - tools/call : 도구 실행 및 결과 반환
 */
UCLASS()
class UMcpServer : public UEditorSubsystem
{
	GENERATED_BODY()
public:
	//-----------------------------------------------------------------------------
	// UEditorSubsystem 오버라이드
	//-----------------------------------------------------------------------------
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Agent Sever를 시작
	void StartServer();
	
	// Agent Server를 종료
	void StopServer();
	
	// Agent Server를 재시작
	void RestartServer();
	
private:
	//-----------------------------------------------------------------------------
	// HTTP 서버
	//-----------------------------------------------------------------------------
	
	// 기존에 실행중인 Agent Server 프로세스를 종료한다
	void KillExistingAgentServer() const;

	// Agent Server 프로세스를 Job Object에 등록하여, 에디터가 비정상 종료되어도
	// Windows가 자동으로 Agent Server를 정리하도록 한다 (Windows 전용)
	void AssignAgentServerToJobObject();
	
	// HTTP 서버를 시작하고 /mcp 앤드포인트를 등록한다
	void StartHttpServer();
	
	// HTTP 서버를 중지하고 라우트를 해제한다
	void StopHttpServer();
	
	//-----------------------------------------------------------------------------
	// JSON-RPC 2.0 라우팅
	//-----------------------------------------------------------------------------
	
	// POST /mcp JSON-RPC 2.0 요청을 처리
	bool HandleMcpRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	
	// initialize 매서드를 처리
	TSharedPtr<FJsonObject> HandleInitialize(int32 RequestId) const;
	
	// tools/list 매서드를 처리
	TSharedPtr<FJsonObject> HandleToolsList(int32 RequestId) const;
	
	// tools/call 매서드를 처리
	TSharedPtr<FJsonObject> HandleToolsCall(int32 RequestId, const TSharedPtr<FJsonObject>& Params);
	
	//-----------------------------------------------------------------------------
	// JSON-RPC 유틸리티
	//-----------------------------------------------------------------------------
	
	// JSON-RPC 2.0 성공 응답을 생성
	static TSharedPtr<FJsonObject> MakeJsonRpcResponse(int32 Id, const TSharedPtr<FJsonObject>& Result);
	
	// JSON-RPC 2.0 에러 응답을 생성
	static TSharedPtr<FJsonObject> MakeJsonRpcError(int32 Id, int32 Code, const FString& Message);
	
	// FJsonObject를 Json 문자열로 직렬화
	static FString SerializeJson(const TSharedPtr<FJsonObject>& JsonObject);
	
	//-----------------------------------------------------------------------------
	// 도구 관리
	//-----------------------------------------------------------------------------
	
	// FMcpTool 파생 구조체를 리플렉션으로 자동 검색하고 inputSchema를 생성
	void DiscoverTools();
	
	// UPROPETY 리플렉션 도구의 inputschema를 생성
	TSharedPtr<FJsonObject> BuildInputSchema(const UScriptStruct* Struct) const;
	
	// 도구를 실행 - ToolParama UPROPERTY에 arguments를 자동 역직렬화 한다
	FMcpResponse ExecuteTool(const FString& Name, const TSharedPtr<FJsonObject>& Arguments);
	
	// ToolParam UPROPERTY에 JSON arguments 값을 설정한다
	void PopulateToolParams(const UScriptStruct* Struct, void* ToolMemory, const TSharedPtr<FJsonObject>& Arguments) const;
	
	// ToolParam 메타에서 JSON 키 이름을 가져온다 (값이 없으면 프로퍼티 이름 사용)
	static FString GetParamJsonKey(const FProperty* Property);
	
private:
	/** 등록된 도구 맵 (도구 이름 → UScriptStruct) */
	UPROPERTY()
	TMap<FString, UScriptStruct*> ToolMap;

	/** tools/list 응답에 사용할 사전 빌드된 도구 정의 목록 */
	TArray<TSharedPtr<FJsonValue>> ToolDefinitions;

	/** HTTP 라우터 */
	TSharedPtr<IHttpRouter> HttpRouter;

	/** 등록된 라우트 핸들 */
	FHttpRouteHandle McpRouteHandle;

	/** Agent Server 프로세스 핸들 */
	FProcHandle AgentServerProcess;

	/** Agent Server 프로세스 ID */
	uint32 AgentServerProcessId = 0;

	/**
	 * 에디터가 크래시/강제종료 등으로 Deinitialize()를 거치지 않고 죽더라도
	 * Windows가 Agent Server를 자동으로 정리하도록 등록해두는 Job Object 핸들.
	 * 이 핸들의 마지막 참조가 닫히면(=에디터 프로세스 종료) 연결된 자식 프로세스도 함께 종료된다.
	 */
	void* AgentServerJobHandle = nullptr;

	/** HTTP 서버 포트 */
	static constexpr uint32 ServerPort = 55559;
	
};
