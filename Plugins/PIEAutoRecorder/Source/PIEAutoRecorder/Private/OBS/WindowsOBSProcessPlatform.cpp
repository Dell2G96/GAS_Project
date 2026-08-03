#include "WindowsOBSProcessPlatform.h"

#include "PIEAutoRecorderLog.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS

// AllowWindowsPlatformTypes.h가 내부적으로 <Windows.h> 전체를 이미 포함하므로, TlHelp32.h는 별도
// Pre/PostWindowsApi 없이 이 사이에서 바로 포함하면 된다(PostMessage 등 매크로는 여기서 이미 해제된 상태다).
#include "Windows/AllowWindowsPlatformTypes.h"
#include <TlHelp32.h>

namespace
{
	// FILETIME을 FDateTime으로 바꾼다. 생성 시각만 비교 목적으로 쓰므로 정밀도는 충분하다.
	FDateTime FileTimeToDateTime(const FILETIME& FileTime)
	{
		ULARGE_INTEGER Large;
		Large.LowPart = FileTime.dwLowDateTime;
		Large.HighPart = FileTime.dwHighDateTime;

		// FILETIME은 1601-01-01 UTC 기준 100나노초 단위다.
		const int64 WindowsEpochTicks = 504911232000000000LL;
		const int64 Ticks = static_cast<int64>(Large.QuadPart) - WindowsEpochTicks;
		return FDateTime(FMath::Max<int64>(Ticks, 0));
	}

	// PID로 프로세스를 열어 CreationTime을 읽는다. 실패하면 기본값(0)을 돌려준다.
	FDateTime QueryCreationTime(uint32 ProcessId)
	{
		HANDLE Handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, ProcessId);
		if (Handle == nullptr)
		{
			return FDateTime();
		}

		FILETIME CreationTime, ExitTime, KernelTime, UserTime;
		FDateTime Result;
		if (::GetProcessTimes(Handle, &CreationTime, &ExitTime, &KernelTime, &UserTime))
		{
			Result = FileTimeToDateTime(CreationTime);
		}

		::CloseHandle(Handle);
		return Result;
	}

	// PID로 프로세스의 전체 실행 경로를 읽는다.
	FString QueryFullImagePath(uint32 ProcessId)
	{
		HANDLE Handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, ProcessId);
		if (Handle == nullptr)
		{
			return FString();
		}

		TCHAR Buffer[MAX_PATH];
		DWORD Size = MAX_PATH;
		FString Result;
		if (::QueryFullProcessImageName(Handle, 0, Buffer, &Size))
		{
			Result = FString(Buffer);
		}

		::CloseHandle(Handle);
		return Result;
	}

	// EnumWindows 콜백에 넘길 대상 PID와 결과 저장소.
	struct FFindWindowContext
	{
		DWORD TargetProcessId = 0;
		TArray<HWND> FoundWindows;
	};

	BOOL CALLBACK EnumWindowsForProcess(HWND Window, LPARAM Param)
	{
		FFindWindowContext* Context = reinterpret_cast<FFindWindowContext*>(Param);

		DWORD WindowProcessId = 0;
		::GetWindowThreadProcessId(Window, &WindowProcessId);

		// 트레이로 최소화된 창도 찾아야 하므로 Visible 여부로 걸러내지 않는다.
		// Owner가 없는(Top-Level) 창만 대상으로 한다.
		if (WindowProcessId == Context->TargetProcessId && ::GetWindow(Window, GW_OWNER) == nullptr)
		{
			Context->FoundWindows.Add(Window);
		}

		return TRUE;
	}
}

// 실행 파일 경로와 일치하는 실행 중 OBS를 찾는다. PID + 전체 경로를 함께 대조한다.
FOBSProcessInfo FWindowsOBSProcessPlatform::FindRunningOBS(const FString& ExecutablePath)
{
	FOBSProcessInfo Result;

	if (ExecutablePath.IsEmpty())
	{
		return Result;
	}

	const FString TargetExeName = FPaths::GetCleanFilename(ExecutablePath);

	HANDLE Snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (Snapshot == INVALID_HANDLE_VALUE)
	{
		return Result;
	}

	PROCESSENTRY32 Entry;
	Entry.dwSize = sizeof(PROCESSENTRY32);

	if (::Process32First(Snapshot, &Entry))
	{
		do
		{
			if (TargetExeName.Equals(FString(Entry.szExeFile), ESearchCase::IgnoreCase))
			{
				// 프로세스 이름만으로 확정하지 않는다. 전체 경로까지 대조한다.
				const FString FullPath = QueryFullImagePath(Entry.th32ProcessID);
				if (!FullPath.IsEmpty() && FPaths::ConvertRelativePathToFull(FullPath).Equals(
						FPaths::ConvertRelativePathToFull(ExecutablePath), ESearchCase::IgnoreCase))
				{
					Result.ProcessId = Entry.th32ProcessID;
					Result.ExecutablePath = ExecutablePath;
					Result.bOwned = false;
					Result.CreationTime = QueryCreationTime(Entry.th32ProcessID);
					Result.Handle = FProcHandle(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, Entry.th32ProcessID));
					break;
				}
			}
		} while (::Process32Next(Snapshot, &Entry));
	}

	::CloseHandle(Snapshot);
	return Result;
}

// OBS를 새로 실행한다. 성공 여부는 Handle과 PID가 유효한지로 판단한다(즉시 종료 여부는 Controller가 확인).
FOBSProcessInfo FWindowsOBSProcessPlatform::LaunchOBS(const FString& ExecutablePath, const FString& Arguments, const FString& WorkingDirectory)
{
	FOBSProcessInfo Result;

	uint32 ProcessId = 0;
	FProcHandle Handle = FPlatformProcess::CreateProc(
		*ExecutablePath,
		*Arguments,
		true,  // bLaunchDetached — 에디터 종료와 OBS 수명을 분리한다.
		false, // bLaunchHidden
		false, // bLaunchReallyHidden
		&ProcessId,
		0,
		WorkingDirectory.IsEmpty() ? nullptr : *WorkingDirectory,
		nullptr);

	if (!Handle.IsValid() || ProcessId == 0)
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("[OBS Process] CreateProc 실패. 경로=%s"), *ExecutablePath);
		return Result;
	}

	Result.Handle = Handle;
	Result.ProcessId = ProcessId;
	Result.ExecutablePath = ExecutablePath;
	Result.bOwned = true;
	Result.CreationTime = QueryCreationTime(ProcessId);

	return Result;
}

// Handle이 가리키는 프로세스가 살아 있는지 확인한다.
bool FWindowsOBSProcessPlatform::IsProcessRunning(const FOBSProcessInfo& ProcessInfo)
{
	if (!ProcessInfo.Handle.IsValid())
	{
		return false;
	}

	return FPlatformProcess::IsProcRunning(const_cast<FProcHandle&>(ProcessInfo.Handle));
}

// 소유 PID와 일치하는 Top-Level Window에만 WM_CLOSE를 보낸다.
bool FWindowsOBSProcessPlatform::RequestGracefulClose(const FOBSProcessInfo& ProcessInfo)
{
	if (!ProcessInfo.IsValid())
	{
		return false;
	}

	FFindWindowContext Context;
	Context.TargetProcessId = static_cast<DWORD>(ProcessInfo.ProcessId);
	::EnumWindows(&EnumWindowsForProcess, reinterpret_cast<LPARAM>(&Context));

	if (Context.FoundWindows.Num() == 0)
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("[OBS Process] 종료 대상 Top-Level Window를 찾지 못했습니다. (PID=%u)"), ProcessInfo.ProcessId);
		return false;
	}

	// UE의 Windows 래퍼가 PostMessage 매크로를 undef하므로(PostMessageW/A 충돌 방지), 직접 W 버전을 호출한다.
	bool bAnySent = false;
	for (HWND Window : Context.FoundWindows)
	{
		if (::PostMessageW(Window, WM_CLOSE, 0, 0))
		{
			bAnySent = true;
		}
	}

	return bAnySent;
}

// Handle을 해제한다. 프로세스 자체는 건드리지 않는다.
void FWindowsOBSProcessPlatform::Release(FOBSProcessInfo& ProcessInfo)
{
	if (ProcessInfo.Handle.IsValid())
	{
		FPlatformProcess::CloseProc(ProcessInfo.Handle);
	}

	ProcessInfo = FOBSProcessInfo();
}

#include "Windows/HideWindowsPlatformTypes.h"

#endif // PLATFORM_WINDOWS
