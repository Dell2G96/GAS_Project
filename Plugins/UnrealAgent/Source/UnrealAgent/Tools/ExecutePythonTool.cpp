#include "Tools/ExecutePythonTool.h"
#include "IPythonScriptPlugin.h"
#include "PythonScriptTypes.h"
#include "Editor.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(ExecutePythonTool)

namespace
{
	// 코드 안에 에디터 맵을 바꾸는 호출이 있는지 검사
	bool ContainsMapLoadCall(const FString& Text)
	{
		static const TCHAR* MapLoadKeywords[] = { TEXT("load_level"), TEXT("load_map"), TEXT("new_level") };
		for (const TCHAR* Keyword : MapLoadKeywords)
		{
			if (Text.Contains(Keyword))
			{
				return true;
			}
		}
		return false;
	}

	// 맵 열기 호출과 import unreal / 주석만 있는 코드인지 검사 (World를 붙잡을 다른 코드가 없어야 안전)
	bool IsMapLoadOnly(const FString& Text)
	{
		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines);
		for (FString Line : Lines)
		{
			Line.TrimStartAndEndInline();
			if (Line.IsEmpty() || Line.StartsWith(TEXT("#")) || Line == TEXT("import unreal"))
			{
				continue;
			}
			if (!ContainsMapLoadCall(Line))
			{
				return false;
			}
		}
		return true;
	}
}

// Python 코드를 실행. 맵 열기는 단독 호출만 허용하고, 그 외 코드는 Undo 트랜잭션으로 감싼다
FMcpResponse FExecutePythonTool::Execute()
{
	// Code는 McpBridge가 ToolParam UPROPERTY에 자동 설정한다
	if (Code.IsEmpty())
		return FMcpResponse::Failure(TEXT("Empty 'code' field"));

	IPythonScriptPlugin* Python = IPythonScriptPlugin::Get();
	if (!Python || !Python->IsPythonAvailable() )
		return FMcpResponse::Failure(TEXT("Python is not available"));

	// 같은 스크립트의 변수가 World를 잡은 채 맵을 열면 에디터가 'World Memory Leaks' Fatal로 죽는다
	const bool bLoadsMap = ContainsMapLoadCall(Code);
	if (bLoadsMap && !IsMapLoadOnly(Code))
	{
		return FMcpResponse::Failure(TEXT(
			"Rejected: map loading (load_level / load_map / new_level) must be in its own execute_python call, "
			"with only `import unreal` and the load call. Mixing it with other code that holds World/asset "
			"references crashes the editor (World Memory Leaks). Split the code into separate calls."));
	}

	// 이전 호출에서 남은 Python 래퍼(순환 참조)가 World를 붙잡지 않도록 맵 로드 전에 먼저 해제
	if (bLoadsMap)
	{
		FPythonCommandEx GcCmd;
		GcCmd.Command = TEXT("import gc; gc.collect()");
		GcCmd.ExecutionMode = EPythonCommandExecutionMode::ExecuteStatement;
		Python->ExecPythonCommandEx(GcCmd);
	}

	// 맵 로드는 Undo 버퍼를 비우므로 트랜잭션으로 감싸지 않는다
	int32 TxIndex = INDEX_NONE;
	if (!bLoadsMap)
	{
		// GEditor 트랜잭션으로 래핑하여 실패 시 자동 롤백
		TxIndex = GEditor->BeginTransaction(FText::FromString(TEXT("UnrealAgent: execute_python")));
	}

	// ExecuteFile 모드 : 코드를 임시 파일처럼 실행하여 여러 Statement 를 지원
	FPythonCommandEx Cmd;
	Cmd.Command = Code;
	Cmd.ExecutionMode = EPythonCommandExecutionMode::ExecuteFile;

	// pythonScriptPlugin을 통해 코드를 실행
	if (Python->ExecPythonCommandEx(Cmd))
	{
		if (!bLoadsMap)
		{
			GEditor->EndTransaction();
		}
		return FMcpResponse::Success(ExtractOutput(Cmd.LogOutput));
	}

	// 실행 실패 시 트랜잭션을 취소하여 에디터 상태를 복원
	if (!bLoadsMap)
	{
		GEditor->CancelTransaction(TxIndex);
	}

	return FMcpResponse::Failure(Cmd.CommandResult);
}

FString FExecutePythonTool::ExtractOutput(const TArray<struct FPythonLogOutputEntry>& LogOutput) const
{
	FString Result;
	
	for (const FPythonLogOutputEntry& Entry : LogOutput)
	{
		if (Entry.Type == EPythonLogOutputType::Info)
		{
			if (!Result.IsEmpty())
			{
				Result += TEXT("\n");
			}
			
			Result += Entry.Output;
		}
	}
	return Result;
}























