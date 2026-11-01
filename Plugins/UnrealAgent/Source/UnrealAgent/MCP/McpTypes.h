#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "McpTypes.generated.h"
//-----------------------------------------------------------------------------
// FMcpResponse
//-----------------------------------------------------------------------------

/*
 * MCP 도구 실행 결과
 */

USTRUCT()
struct FMcpResponse
{
	GENERATED_BODY()
	
	// 실행 성공 여부
	UPROPERTY()
	bool bSuccess = false;
	
	// 성공 시 결과 문자열
	UPROPERTY()
	FString Result;
	
	// 실패 시 에러 메시지
	UPROPERTY()
	FString Error;
	
	// 성공 응답을 생성
	static FMcpResponse Success(const FString& InResult)
	{
		FMcpResponse Response;
		Response.bSuccess = true;
		Response.Result = InResult;
		
		return Response;
	}
	
	// 실패 응답을 생성
	static FMcpResponse Failure(const FString& InError)
	{
		FMcpResponse Response;
		Response.bSuccess = false;
		Response.Result = InError;
		
		return Response;
	}
	
	// 결과 텍스트를 반환 (성공이면 Result, 실패면 Error)
	const FString& GetText() const
	{
		return bSuccess? Result : Error;
	}
};


//-----------------------------------------------------------------------------
// FMcpTool
//-----------------------------------------------------------------------------

USTRUCT(meta=(Hidden))
struct FMcpTool
{
	GENERATED_BODY()

virtual ~FMcpTool() = default;
	
	// 도구 인자 원본
	/*
	 * McpBridge 가 Execute() 호출 전에 ToolParam UPROPETY를 자동 설정하므로 일반적으로는 접근할 필요가 없다
	 */
	TSharedPtr<FJsonObject> Args;
	
	// MCP 도구 설명 반환 ( Claude 에게 전달)
	virtual FString ToolDescription() const {return TEXT("");}
	
	// 도구를 실행하고 결과를 반환
	virtual FMcpResponse Execute()
	{
		return FMcpResponse::Failure(TEXT("Not implemented"));
	}
	
};






















