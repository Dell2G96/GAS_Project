#include "Misc/AutomationTest.h"

#include "PIERecordingCoordinator.h"
#include "IPIERecordingBackend.h"
#include "Disposition/RecordingDispositionQueue.h"
#include "PIEAutoRecorderSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PIEAutoRecorderShutdownTest
{
	// 응답을 즉시 동기적으로 돌려주는 Fake 백엔드. 실제 WebSocket 없이 Coordinator의 상태 전이만 검증한다.
	class FFakeRecordingBackend : public IPIERecordingBackend
	{
	public:
		bool bReady = true;
		bool bQuerySuccess = true;
		bool bOutputActive = false;
		bool bStartSuccess = true;
		bool bStopSuccess = true;
		FString StopOutputPath;

		int32 QueryCount = 0;
		int32 StartCount = 0;
		int32 StopCount = 0;

		virtual void Connect() override {}
		virtual void Disconnect() override {}
		virtual bool IsReady() const override { return bReady; }

		virtual void QueryRecordStatus(FGuid SessionId, FOnRecordStatusResult Callback) override
		{
			QueryCount++;
			FOBSRecordStatus Status;
			Status.bOutputActive = bOutputActive;
			Callback.ExecuteIfBound(bQuerySuccess, Status, SessionId);
		}

		virtual void StartRecording(FGuid SessionId, FOnSimpleResult Callback) override
		{
			StartCount++;
			Callback.ExecuteIfBound(bStartSuccess, TEXT(""), SessionId);
		}

		virtual void StopRecording(FGuid SessionId, FOnStopResult Callback) override
		{
			StopCount++;
			Callback.ExecuteIfBound(bStopSuccess, bStopSuccess ? StopOutputPath : FString(), SessionId);
		}

		virtual FOnRecordStateChanged& OnRecordStateChanged() override { return Event; }

	private:
		FOnRecordStateChanged Event;
	};

	// bEnableAutoRecording 등 필요한 설정을 테스트 동안만 켜고 소멸자에서 원복한다.
	class FScopedRecordingSettings
	{
	public:
		FScopedRecordingSettings()
		{
			Settings = GetMutableDefault<UPIEAutoRecorderSettings>();
			bPreviousEnable = Settings->bEnableAutoRecording;
			PreviousStopDelay = Settings->StopDelaySeconds;

			Settings->bEnableAutoRecording = true;
			Settings->StopDelaySeconds = 0.0f;
		}

		~FScopedRecordingSettings()
		{
			Settings->bEnableAutoRecording = bPreviousEnable;
			Settings->StopDelaySeconds = PreviousStopDelay;
		}

	private:
		UPIEAutoRecorderSettings* Settings = nullptr;
		bool  bPreviousEnable = false;
		float PreviousStopDelay = 0.0f;
	};

	// 종료 준비 콜백의 결과를 받아 두는 헬퍼.
	struct FShutdownResult
	{
		bool bResolved = false;
		bool bReady = false;
		EOBSShutdownBlockReason Reason = EOBSShutdownBlockReason::InvalidState;
		FString Message;
	};
}

//========================================================================================
// Idle + 녹화 없음 -> Ready
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSShutdownIdleReadyTest, "PIEAutoRecorder.OBSShutdown.IdleReady",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOBSShutdownIdleReadyTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderShutdownTest;

	TSharedRef<FFakeRecordingBackend> Backend = MakeShared<FFakeRecordingBackend>();
	Backend->bOutputActive = false;
	TSharedRef<FRecordingDispositionQueue> Disposition = MakeShared<FRecordingDispositionQueue>();

	FPIERecordingCoordinator Coordinator(Backend, Disposition);

	FShutdownResult Result;
	Coordinator.PrepareForOBSShutdown(
		FOnOBSShutdownReady::CreateLambda([&Result]() { Result.bResolved = true; Result.bReady = true; }),
		FOnOBSShutdownRejected::CreateLambda([&Result](EOBSShutdownBlockReason Reason, FString Message)
		{
			Result.bResolved = true;
			Result.bReady = false;
			Result.Reason = Reason;
			Result.Message = Message;
		}));

	TestTrue(TEXT("콜백이 실행됨"), Result.bResolved);
	TestTrue(TEXT("녹화가 없으므로 Ready"), Result.bReady);
	TestFalse(TEXT("종료 준비 상태가 남아있지 않음"), Coordinator.IsOBSShutdownPending());

	return true;
}

//========================================================================================
// Idle + 외부 녹화 감지 -> Rejected(ExternalRecording)
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSShutdownExternalRecordingTest, "PIEAutoRecorder.OBSShutdown.ExternalRecordingRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOBSShutdownExternalRecordingTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderShutdownTest;

	TSharedRef<FFakeRecordingBackend> Backend = MakeShared<FFakeRecordingBackend>();
	Backend->bOutputActive = true; // 우리가 시작하지 않은 녹화가 이미 돌고 있다.
	TSharedRef<FRecordingDispositionQueue> Disposition = MakeShared<FRecordingDispositionQueue>();

	FPIERecordingCoordinator Coordinator(Backend, Disposition);

	FShutdownResult Result;
	Coordinator.PrepareForOBSShutdown(
		FOnOBSShutdownReady::CreateLambda([&Result]() { Result.bResolved = true; Result.bReady = true; }),
		FOnOBSShutdownRejected::CreateLambda([&Result](EOBSShutdownBlockReason Reason, FString Message)
		{
			Result.bResolved = true;
			Result.bReady = false;
			Result.Reason = Reason;
		}));

	TestTrue(TEXT("콜백이 실행됨"), Result.bResolved);
	TestFalse(TEXT("외부 녹화이므로 거부"), Result.bReady);
	TestEqual(TEXT("거부 사유는 ExternalRecording"),
		static_cast<uint8>(Result.Reason), static_cast<uint8>(EOBSShutdownBlockReason::ExternalRecording));

	return true;
}

//========================================================================================
// Backend 연결 없음 -> Rejected(BackendUnavailable)
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSShutdownBackendUnavailableTest, "PIEAutoRecorder.OBSShutdown.BackendUnavailableRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOBSShutdownBackendUnavailableTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderShutdownTest;

	TSharedRef<FFakeRecordingBackend> Backend = MakeShared<FFakeRecordingBackend>();
	Backend->bReady = false;
	TSharedRef<FRecordingDispositionQueue> Disposition = MakeShared<FRecordingDispositionQueue>();

	FPIERecordingCoordinator Coordinator(Backend, Disposition);

	FShutdownResult Result;
	Coordinator.PrepareForOBSShutdown(
		FOnOBSShutdownReady::CreateLambda([&Result]() { Result.bResolved = true; Result.bReady = true; }),
		FOnOBSShutdownRejected::CreateLambda([&Result](EOBSShutdownBlockReason Reason, FString Message)
		{
			Result.bResolved = true;
			Result.bReady = false;
			Result.Reason = Reason;
		}));

	TestTrue(TEXT("콜백이 실행됨"), Result.bResolved);
	TestFalse(TEXT("연결이 없으므로 거부"), Result.bReady);
	TestEqual(TEXT("거부 사유는 BackendUnavailable"),
		static_cast<uint8>(Result.Reason), static_cast<uint8>(EOBSShutdownBlockReason::BackendUnavailable));

	return true;
}

//========================================================================================
// RecordingOwned -> Stop 성공 -> Ready
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSShutdownRecordingOwnedStopSuccessTest, "PIEAutoRecorder.OBSShutdown.RecordingOwnedStopSuccess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOBSShutdownRecordingOwnedStopSuccessTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderShutdownTest;

	FScopedRecordingSettings ScopedSettings;

	TSharedRef<FFakeRecordingBackend> Backend = MakeShared<FFakeRecordingBackend>();
	Backend->bOutputActive = false;
	Backend->bStopSuccess = true;
	Backend->StopOutputPath = FString(); // 비워서 Disposition 큐(Slate 창)로 넘어가지 않게 한다.
	TSharedRef<FRecordingDispositionQueue> Disposition = MakeShared<FRecordingDispositionQueue>();

	FPIERecordingCoordinator Coordinator(Backend, Disposition);

	// PIE 시작 -> 조회 -> 시작까지 동기적으로 진행되어 RecordingOwned가 된다.
	Coordinator.HandlePostPIEStarted(false);
	TestEqual(TEXT("녹화 시작됨"), Coordinator.GetStateDescription(), FString(TEXT("녹화 중 (우리 소유)")));

	FShutdownResult Result;
	Coordinator.PrepareForOBSShutdown(
		FOnOBSShutdownReady::CreateLambda([&Result]() { Result.bResolved = true; Result.bReady = true; }),
		FOnOBSShutdownRejected::CreateLambda([&Result](EOBSShutdownBlockReason Reason, FString Message)
		{
			Result.bResolved = true;
			Result.bReady = false;
		}));

	TestTrue(TEXT("콜백이 실행됨"), Result.bResolved);
	TestTrue(TEXT("정지 성공 후 Ready"), Result.bReady);
	TestEqual(TEXT("StopRecording이 1회 호출됨"), Backend->StopCount, 1);

	return true;
}

//========================================================================================
// RecordingOwned -> Stop 실패 -> Rejected(StopRecordingFailed)
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSShutdownRecordingOwnedStopFailTest, "PIEAutoRecorder.OBSShutdown.RecordingOwnedStopFailed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOBSShutdownRecordingOwnedStopFailTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderShutdownTest;

	FScopedRecordingSettings ScopedSettings;

	TSharedRef<FFakeRecordingBackend> Backend = MakeShared<FFakeRecordingBackend>();
	Backend->bOutputActive = false;
	Backend->bStopSuccess = false;
	TSharedRef<FRecordingDispositionQueue> Disposition = MakeShared<FRecordingDispositionQueue>();

	FPIERecordingCoordinator Coordinator(Backend, Disposition);

	Coordinator.HandlePostPIEStarted(false);
	TestEqual(TEXT("녹화 시작됨"), Coordinator.GetStateDescription(), FString(TEXT("녹화 중 (우리 소유)")));

	FShutdownResult Result;
	Coordinator.PrepareForOBSShutdown(
		FOnOBSShutdownReady::CreateLambda([&Result]() { Result.bResolved = true; Result.bReady = true; }),
		FOnOBSShutdownRejected::CreateLambda([&Result](EOBSShutdownBlockReason Reason, FString Message)
		{
			Result.bResolved = true;
			Result.bReady = false;
			Result.Reason = Reason;
		}));

	TestTrue(TEXT("콜백이 실행됨"), Result.bResolved);
	TestFalse(TEXT("정지 실패로 거부"), Result.bReady);
	TestEqual(TEXT("거부 사유는 StopRecordingFailed"),
		static_cast<uint8>(Result.Reason), static_cast<uint8>(EOBSShutdownBlockReason::StopRecordingFailed));

	return true;
}

//========================================================================================
// 종료 준비 중(bOBSShutdownPending=true) 두 번째 요청 -> 무시되고 콜백은 실행되지 않는다.
// Fake 백엔드가 동기 응답이라 StopPending을 직접 재현할 수 없으므로, bOBSShutdownPending을
// 테스트에서 직접 관찰 가능한 API(IsOBSShutdownPending)로 "완료 후에는 재요청 가능"함을 검증한다.
//========================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOBSShutdownSequentialRequestsTest, "PIEAutoRecorder.OBSShutdown.SequentialRequestsEachResolve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOBSShutdownSequentialRequestsTest::RunTest(const FString& Parameters)
{
	using namespace PIEAutoRecorderShutdownTest;

	FScopedRecordingSettings ScopedSettings;

	TSharedRef<FFakeRecordingBackend> Backend = MakeShared<FFakeRecordingBackend>();
	Backend->bOutputActive = false;
	TSharedRef<FRecordingDispositionQueue> Disposition = MakeShared<FRecordingDispositionQueue>();

	FPIERecordingCoordinator Coordinator(Backend, Disposition);

	Coordinator.HandlePostPIEStarted(false);

	int32 ReadyCount = 0;

	Coordinator.PrepareForOBSShutdown(
		FOnOBSShutdownReady::CreateLambda([&ReadyCount]() { ReadyCount++; }),
		FOnOBSShutdownRejected::CreateLambda([](EOBSShutdownBlockReason, FString) {}));

	TestFalse(TEXT("첫 요청이 즉시 완료돼 대기 상태가 남지 않음"), Coordinator.IsOBSShutdownPending());
	TestEqual(TEXT("첫 요청 Ready 1회"), ReadyCount, 1);

	// 첫 요청이 이미 끝났으므로 두 번째 요청은 새 요청으로 정상 처리된다(중복이 아니라 순차 요청).
	Coordinator.PrepareForOBSShutdown(
		FOnOBSShutdownReady::CreateLambda([&ReadyCount]() { ReadyCount++; }),
		FOnOBSShutdownRejected::CreateLambda([](EOBSShutdownBlockReason, FString) {}));

	TestEqual(TEXT("두 번째 요청도 Ready로 처리됨"), ReadyCount, 2);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
