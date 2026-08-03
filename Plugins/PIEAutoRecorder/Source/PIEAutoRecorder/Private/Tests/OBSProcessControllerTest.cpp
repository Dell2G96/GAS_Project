#include "Misc/AutomationTest.h"

#include "OBS/OBSProcessController.h"
#include "OBS/IOBSProcessPlatform.h"
#include "PIEAutoRecorderSettings.h"
#include "Containers/Ticker.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PIEAutoRecorderProcessTest
{
	// 실제 OBS를 실행하지 않는 Fake 플랫폼. ProcessId를 키로 생존 여부를 직접 조작해 상태 전이를 재현한다.
	class FFakeOBSProcessPlatform : public IOBSProcessPlatform
	{
	public:
		bool  bShouldFindExternal = false;
		bool  bLaunchShouldSucceed = true;
		bool  bGracefulCloseShouldSucceed = true;
		int32 LaunchCount = 0;
		int32 GracefulCloseCount = 0;

		uint32 NextProcessId = 1000;
		TSet<uint32> RunningIds;

		virtual FOBSProcessInfo FindRunningOBS(const FString& ExecutablePath) override
		{
			if (!bShouldFindExternal)
			{
				return FOBSProcessInfo();
			}

			FOBSProcessInfo Info;
			Info.ProcessId = NextProcessId++;
			Info.ExecutablePath = ExecutablePath;
			Info.bOwned = false;
			RunningIds.Add(Info.ProcessId);
			return Info;
		}

		virtual FOBSProcessInfo LaunchOBS(const FString& ExecutablePath, const FString& Arguments, const FString& WorkingDirectory) override
		{
			LaunchCount++;

			if (!bLaunchShouldSucceed)
			{
				return FOBSProcessInfo();
			}

			FOBSProcessInfo Info;
			Info.ProcessId = NextProcessId++;
			Info.ExecutablePath = ExecutablePath;
			Info.bOwned = true;
			RunningIds.Add(Info.ProcessId);
			return Info;
		}

		virtual bool IsProcessRunning(const FOBSProcessInfo& ProcessInfo) override
		{
			return RunningIds.Contains(ProcessInfo.ProcessId);
		}

		virtual bool RequestGracefulClose(const FOBSProcessInfo& ProcessInfo) override
		{
			GracefulCloseCount++;
			return bGracefulCloseShouldSucceed;
		}

		virtual void Release(FOBSProcessInfo& ProcessInfo) override
		{
			ProcessInfo = FOBSProcessInfo();
		}

		// 테스트 전용: 프로세스가 스스로(또는 정상 종료로) 사라졌음을 시뮬레이션한다.
		void SimulateProcessExit(uint32 ProcessId)
		{
			RunningIds.Remove(ProcessId);
		}
	};

	// 테스트 동안만 쓸 더미 실행 파일을 만들고 설정에 반영한다. 소멸자에서 설정과 파일을 원복한다.
	class FScopedFakeExecutable
	{
	public:
		FScopedFakeExecutable()
		{
			Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PIEAutoRecorderTest"), FGuid::NewGuid().ToString());
			IFileManager::Get().MakeDirectory(*Directory, /*Tree=*/true);
			DummyExePath = FPaths::Combine(Directory, TEXT("fake_obs.exe"));
			FFileHelper::SaveStringToFile(TEXT("dummy"), *DummyExePath);

			Settings = GetMutableDefault<UPIEAutoRecorderSettings>();
			PreviousPath = Settings->OBSExecutablePath.FilePath;
			PreviousStartupTimeout = Settings->OBSStartupTimeoutSeconds;
			PreviousShutdownTimeout = Settings->OBSShutdownTimeoutSeconds;

			Settings->OBSExecutablePath.FilePath = DummyExePath;
			Settings->OBSStartupTimeoutSeconds = 0.01f;
			Settings->OBSShutdownTimeoutSeconds = 0.05f;
		}

		~FScopedFakeExecutable()
		{
			Settings->OBSExecutablePath.FilePath = PreviousPath;
			Settings->OBSStartupTimeoutSeconds = PreviousStartupTimeout;
			Settings->OBSShutdownTimeoutSeconds = PreviousShutdownTimeout;

			IFileManager::Get().DeleteDirectory(*Directory, false, true);
		}

		FString GetPath() const { return DummyExePath; }

	private:
		UPIEAutoRecorderSettings* Settings = nullptr;
		FString PreviousPath;
		float   PreviousStartupTimeout = 10.0f;
		float   PreviousShutdownTimeout = 10.0f;
		FString Directory;
		FString DummyExePath;
	};

	// Ticker에 등록된 Controller의 Monitor를 강제로 실행시킨다.
	void ForceTick()
	{
		FTSTicker::GetCoreTicker().Tick(1.0f);
	}

	// TestEqual이 enum class를 별도 ToString 없이 비교할 수 있도록 정수로 바꾼다.
	uint8 S(EOBSProcessState State)
	{
		return static_cast<uint8>(State);
	}
}

//========================================================================================
// Stopped -> Starting -> RunningOwned
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSProcessStartOwnedTest, "PIEAutoRecorder.OBSProcess.StartOwned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOBSProcessStartOwnedTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderProcessTest;

	FScopedFakeExecutable Fixture;

	TSharedRef<FFakeOBSProcessPlatform> Fake = MakeShared<FFakeOBSProcessPlatform>();
	FOBSProcessController Controller(Fake);
	Controller.Initialize();

	TestEqual(TEXT("초기 상태는 Stopped"), S(Controller.GetState()), S(EOBSProcessState::Stopped));

	Controller.StartOBS();
	TestEqual(TEXT("실행 요청 직후 Starting"), S(Controller.GetState()), S(EOBSProcessState::Starting));
	TestEqual(TEXT("Launch가 1회 호출됨"), Fake->LaunchCount, 1);

	FPlatformProcess::Sleep(0.02f);
	ForceTick();
	TestEqual(TEXT("생존 확인 후 RunningOwned"), S(Controller.GetState()), S(EOBSProcessState::RunningOwned));

	Controller.Shutdown();
	return true;
}

//========================================================================================
// Stopped -> RunningExternal, RunningExternal -> Stopped(외부 종료 감지)
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSProcessExternalTest, "PIEAutoRecorder.OBSProcess.ExternalDetectAndExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOBSProcessExternalTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderProcessTest;

	FScopedFakeExecutable Fixture;

	TSharedRef<FFakeOBSProcessPlatform> Fake = MakeShared<FFakeOBSProcessPlatform>();
	Fake->bShouldFindExternal = true;

	FOBSProcessController Controller(Fake);
	Controller.Initialize();

	Controller.StartOBS();
	TestEqual(TEXT("외부 OBS 발견 시 RunningExternal"), S(Controller.GetState()), S(EOBSProcessState::RunningExternal));
	TestEqual(TEXT("외부 OBS는 실행하지 않음"), Fake->LaunchCount, 0);

	// 외부 OBS가 종료됨을 시뮬레이션.
	const uint32 ExternalId = Fake->NextProcessId - 1;
	Fake->SimulateProcessExit(ExternalId);
	ForceTick();

	TestEqual(TEXT("외부 종료 감지 후 Stopped"), S(Controller.GetState()), S(EOBSProcessState::Stopped));

	Controller.Shutdown();
	return true;
}

//========================================================================================
// RunningOwned -> Closing -> Stopped (정상 종료)
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSProcessGracefulCloseTest, "PIEAutoRecorder.OBSProcess.GracefulClose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOBSProcessGracefulCloseTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderProcessTest;

	FScopedFakeExecutable Fixture;

	TSharedRef<FFakeOBSProcessPlatform> Fake = MakeShared<FFakeOBSProcessPlatform>();
	FOBSProcessController Controller(Fake);
	Controller.Initialize();

	Controller.StartOBS();
	FPlatformProcess::Sleep(0.02f);
	ForceTick();
	TestEqual(TEXT("실행 성공"), S(Controller.GetState()), S(EOBSProcessState::RunningOwned));

	const uint32 OwnedId = Fake->NextProcessId - 1;

	Controller.MarkPreparingShutdown();
	TestEqual(TEXT("PreparingShutdown 전이"), S(Controller.GetState()), S(EOBSProcessState::PreparingShutdown));

	Controller.RequestCloseOwnedOBS();
	TestEqual(TEXT("Closing 전이"), S(Controller.GetState()), S(EOBSProcessState::Closing));
	TestEqual(TEXT("정상 종료 메시지 1회 전송"), Fake->GracefulCloseCount, 1);

	// OBS가 정상적으로 종료됐다고 가정.
	Fake->SimulateProcessExit(OwnedId);
	ForceTick();
	TestEqual(TEXT("종료 완료 후 Stopped"), S(Controller.GetState()), S(EOBSProcessState::Stopped));

	Controller.Shutdown();
	return true;
}

//========================================================================================
// Closing -> Timeout -> RunningOwned (강제 종료하지 않음)
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSProcessCloseTimeoutTest, "PIEAutoRecorder.OBSProcess.CloseTimeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOBSProcessCloseTimeoutTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderProcessTest;

	FScopedFakeExecutable Fixture;

	TSharedRef<FFakeOBSProcessPlatform> Fake = MakeShared<FFakeOBSProcessPlatform>();
	FOBSProcessController Controller(Fake);
	Controller.Initialize();

	Controller.StartOBS();
	FPlatformProcess::Sleep(0.02f);
	ForceTick();
	Controller.MarkPreparingShutdown();
	Controller.RequestCloseOwnedOBS();
	TestEqual(TEXT("Closing 전이"), S(Controller.GetState()), S(EOBSProcessState::Closing));

	// 프로세스를 살려둔 채로(=응답 없음) 짧은 ShutdownTimeout(0.05초)을 넘긴다.
	FPlatformProcess::Sleep(0.1f);
	ForceTick();

	TestEqual(TEXT("Timeout 후 RunningOwned로 복구"), S(Controller.GetState()), S(EOBSProcessState::RunningOwned));
	TestTrue(TEXT("강제 종료를 호출하지 않아 여전히 실행 중"), Controller.IsOwnedProcessRunning());

	Controller.Shutdown();
	return true;
}

//========================================================================================
// RunningOwned -> 예상치 못한 종료 -> Stopped
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSProcessUnexpectedExitTest, "PIEAutoRecorder.OBSProcess.UnexpectedExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOBSProcessUnexpectedExitTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderProcessTest;

	FScopedFakeExecutable Fixture;

	TSharedRef<FFakeOBSProcessPlatform> Fake = MakeShared<FFakeOBSProcessPlatform>();
	FOBSProcessController Controller(Fake);
	Controller.Initialize();

	Controller.StartOBS();
	FPlatformProcess::Sleep(0.02f);
	ForceTick();
	TestEqual(TEXT("실행 성공"), S(Controller.GetState()), S(EOBSProcessState::RunningOwned));

	const uint32 OwnedId = Fake->NextProcessId - 1;
	Fake->SimulateProcessExit(OwnedId);
	ForceTick();

	TestEqual(TEXT("예상치 못한 종료 감지 후 Stopped"), S(Controller.GetState()), S(EOBSProcessState::Stopped));

	Controller.Shutdown();
	return true;
}

//========================================================================================
// 경로 없음 -> Failed, Handle 없음
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSProcessMissingPathTest, "PIEAutoRecorder.OBSProcess.MissingPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOBSProcessMissingPathTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderProcessTest;

	UPIEAutoRecorderSettings* Settings = GetMutableDefault<UPIEAutoRecorderSettings>();
	const FString PreviousPath = Settings->OBSExecutablePath.FilePath;
	Settings->OBSExecutablePath.FilePath = TEXT("");

	TSharedRef<FFakeOBSProcessPlatform> Fake = MakeShared<FFakeOBSProcessPlatform>();
	FOBSProcessController Controller(Fake);
	Controller.Initialize();

	Controller.StartOBS();
	TestEqual(TEXT("경로 없으면 Failed"), S(Controller.GetState()), S(EOBSProcessState::Failed));
	TestEqual(TEXT("실행 시도 자체가 없음"), Fake->LaunchCount, 0);
	TestFalse(TEXT("소유 프로세스 없음"), Controller.IsOwnedProcessRunning());

	Controller.Shutdown();
	Settings->OBSExecutablePath.FilePath = PreviousPath;
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
