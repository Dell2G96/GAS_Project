#include "PIEAutoRecorderModule.h"

#include "PIEAutoRecorderLog.h"
#include "PIEAutoRecorderSettings.h"
#include "OBS/OBSWebSocketBackend.h"
#include "PIERecordingCoordinator.h"
#include "Disposition/RecordingDispositionQueue.h"
#include "Editor.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogPIEAutoRecorder);

#define LOCTEXT_NAMESPACE "FPIEAutoRecorderModule"

// 모듈 시작. 백엔드를 만들고, 설정이 허용하면 미리 연결해 둔다.
void FPIEAutoRecorderModule::StartupModule()
{
	UE_LOG(LogPIEAutoRecorder, Log, TEXT("PIEAutoRecorder 모듈을 시작합니다."));

	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	if (Settings)
	{
		Settings->LogCurrentSettings();
	}

	Backend = MakeShared<FOBSWebSocketBackend>();
	Backend->OnRecordStateChanged().AddRaw(this, &FPIEAutoRecorderModule::HandleRecordStateChanged);

	DispositionQueue = MakeShared<FRecordingDispositionQueue>();
	Coordinator = MakeUnique<FPIERecordingCoordinator>(Backend.ToSharedRef(), DispositionQueue.ToSharedRef());

	RegisterPIEDelegates();
	RegisterConsoleCommands();

	// 설정 화면에서 값을 바꾸면 즉시 반영한다. 에디터를 다시 켜지 않아도 되도록.
	SettingsChangedHandle = GetMutableDefault<UPIEAutoRecorderSettings>()->OnSettingChanged()
		.AddRaw(this, &FPIEAutoRecorderModule::HandleSettingsChanged);

	// 사전 연결로 PIE 시작 지연을 줄인다. bEnableAutoRecording이 꺼져 있으면 Connect가 스스로 아무것도 하지 않는다.
	if (Settings && Settings->bConnectWhenEditorStarts)
	{
		Backend->Connect();
	}
}

// 모듈 종료. 등록한 것을 대칭적으로 해제한다. 하나라도 빠지면 핫 리로드에서 중복 등록이 남는다.
void FPIEAutoRecorderModule::ShutdownModule()
{
	if (SettingsChangedHandle.IsValid())
	{
		GetMutableDefault<UPIEAutoRecorderSettings>()->OnSettingChanged().Remove(SettingsChangedHandle);
		SettingsChangedHandle.Reset();
	}

	UnregisterPIEDelegates();
	UnregisterConsoleCommands();

	// Coordinator를 먼저 없앤다. 백엔드 콜백이 사라진 Coordinator를 부르지 못하게 하기 위함이다.
	Coordinator.Reset();
	DispositionQueue.Reset();

	if (Backend.IsValid())
	{
		Backend->OnRecordStateChanged().RemoveAll(this);
		Backend->Disconnect();
		Backend.Reset();
	}

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("PIEAutoRecorder 모듈을 종료합니다."));
}

// 녹화 상태 변화 이벤트를 로그로 남긴다. 4단계에서는 Coordinator가 이 정보를 보조로 쓴다.
void FPIEAutoRecorderModule::HandleRecordStateChanged(const FOBSRecordStateChanged& Event)
{
	UE_LOG(LogPIEAutoRecorder, Log, TEXT("[이벤트] 녹화 상태=%s, active=%s, path=%s"),
		*Event.OutputState,
		Event.bOutputActive ? TEXT("true") : TEXT("false"),
		Event.OutputPath.IsEmpty() ? TEXT("(없음)") : *Event.OutputPath);
}

// PIE 델리게이트 5종을 Coordinator에 연결한다.
// PreBeginPIE는 쓰지 않는다. 연결은 에디터 시작 시 미리 해두므로 사전 점검 지점이 필요 없다.
void FPIEAutoRecorderModule::RegisterPIEDelegates()
{
	FPIERecordingCoordinator* Raw = Coordinator.Get();
	if (Raw == nullptr)
	{
		return;
	}

	PostPIEStartedHandle = FEditorDelegates::PostPIEStarted.AddLambda([Raw](bool bIsSimulating)
	{
		Raw->HandlePostPIEStarted(bIsSimulating);
	});

	PrePIEEndedHandle = FEditorDelegates::PrePIEEnded.AddLambda([Raw](bool bIsSimulating)
	{
		Raw->HandlePrePIEEnded(bIsSimulating);
	});

	ShutdownPIEHandle = FEditorDelegates::ShutdownPIE.AddLambda([Raw](bool bIsSimulating)
	{
		Raw->HandleShutdownPIE(bIsSimulating);
	});

	// CancelPIE와 OnEditorPreExit는 인자가 없다.
	CancelPIEHandle = FEditorDelegates::CancelPIE.AddLambda([Raw]()
	{
		Raw->HandleCancelPIE();
	});

	FRecordingDispositionQueue* RawQueue = DispositionQueue.Get();

	EditorPreExitHandle = FEditorDelegates::OnEditorPreExit.AddLambda([Raw, RawQueue]()
	{
		// 소유 녹화 정지를 먼저 시도하고, 그다음 미결 저장 항목을 정리한다.
		Raw->HandleEditorPreExit();

		if (RawQueue)
		{
			RawQueue->HandleEditorPreExit();
		}
	});
}

// 등록한 PIE 델리게이트를 전부 해제한다. Live Coding 중복 등록은 이 대칭성으로 막는다.
void FPIEAutoRecorderModule::UnregisterPIEDelegates()
{
	FEditorDelegates::PostPIEStarted.Remove(PostPIEStartedHandle);
	FEditorDelegates::PrePIEEnded.Remove(PrePIEEndedHandle);
	FEditorDelegates::ShutdownPIE.Remove(ShutdownPIEHandle);
	FEditorDelegates::CancelPIE.Remove(CancelPIEHandle);
	FEditorDelegates::OnEditorPreExit.Remove(EditorPreExitHandle);

	PostPIEStartedHandle.Reset();
	PrePIEEndedHandle.Reset();
	ShutdownPIEHandle.Reset();
	CancelPIEHandle.Reset();
	EditorPreExitHandle.Reset();
}

// 설정이 바뀌었다. 연결에 영향을 주는 항목만 골라 안전할 때 다시 연결한다.
void FPIEAutoRecorderModule::HandleSettingsChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!Backend.IsValid())
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	// 연결과 무관한 설정(저장 창, 알림 등)은 다음 사용 시점에 자동으로 반영되므로 아무것도 하지 않는다.
	static const TSet<FName> ConnectionProperties =
	{
		GET_MEMBER_NAME_CHECKED(UPIEAutoRecorderSettings, bEnableAutoRecording),
		GET_MEMBER_NAME_CHECKED(UPIEAutoRecorderSettings, ServerHost),
		GET_MEMBER_NAME_CHECKED(UPIEAutoRecorderSettings, ServerPort),
		GET_MEMBER_NAME_CHECKED(UPIEAutoRecorderSettings, Password),
	};

	if (!ConnectionProperties.Contains(PropertyName))
	{
		return;
	}

	// 녹화 중에 연결을 끊으면 정지 요청을 보낼 수 없게 된다. 그때는 손대지 않는다.
	if (Coordinator.IsValid() && Coordinator->IsBusy())
	{
		UE_LOG(LogPIEAutoRecorder, Warning,
			TEXT("녹화가 진행 중이라 연결 설정을 지금 적용하지 않습니다. PIE를 끝낸 뒤 PIEAutoRecorder.Connect를 실행하세요."));
		return;
	}

	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("연결 설정이 바뀌어 다시 연결합니다. (%s)"), *PropertyName.ToString());

	Backend->Disconnect();

	if (Settings && Settings->bEnableAutoRecording)
	{
		Backend->Connect();
	}
}

// 콘솔 명령을 실행할 수 있는 상태인지 확인한다.
// 자동 연결과 달리 사용자가 직접 친 명령은 조용히 무시하지 않고 이유와 해결 방법을 알려준다.
bool FPIEAutoRecorderModule::CanRunConsoleCommand() const
{
	if (!Backend.IsValid())
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("백엔드가 아직 준비되지 않았습니다."));
		return false;
	}

	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	if (Settings == nullptr || !Settings->bEnableAutoRecording)
	{
		// config가 EditorPerProjectUserSettings라 Project Settings가 아니라 Editor Preferences에 표시된다.
		UE_LOG(LogPIEAutoRecorder, Warning,
			TEXT("자동 녹화가 꺼져 있습니다. Edit > Editor Preferences > Plugins > PIE Auto Recorder > 동작 에서 Enable Auto Recording을 켜세요."));
		return false;
	}

	return true;
}

// 3단계 수동 검증용 콘솔 명령 6개를 등록한다. Coordinator가 붙으면 이 명령들은 진단용으로만 남는다.
void FPIEAutoRecorderModule::RegisterConsoleCommands()
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	ConsoleCommands.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("PIEAutoRecorder.Connect"),
		TEXT("OBS WebSocket에 연결을 시도합니다."),
		FConsoleCommandDelegate::CreateLambda([this]()
		{
			if (!CanRunConsoleCommand())
			{
				return;
			}

			Backend->Connect();
		})));

	ConsoleCommands.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("PIEAutoRecorder.Disconnect"),
		TEXT("OBS WebSocket 연결을 해제합니다."),
		FConsoleCommandDelegate::CreateLambda([this]()
		{
			if (Backend.IsValid())
			{
				Backend->Disconnect();
			}
		})));

	ConsoleCommands.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("PIEAutoRecorder.Status"),
		TEXT("현재 설정과 연결 상태를 출력합니다."),
		FConsoleCommandDelegate::CreateLambda([this]()
		{
			// 진단용이므로 자동 녹화가 꺼져 있어도 항상 동작한다. 오히려 그 사실을 보여주는 것이 목적이다.
			const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
			if (Settings)
			{
				UE_LOG(LogPIEAutoRecorder, Display, TEXT("자동 녹화: %s / 서버: %s:%d / 비밀번호: %s"),
					Settings->bEnableAutoRecording ? TEXT("켜짐") : TEXT("꺼짐 (연결하려면 켜야 합니다)"),
					*Settings->ServerHost,
					Settings->ServerPort,
					Settings->ResolvePassword().IsEmpty() ? TEXT("(없음)") : TEXT("(설정됨)"));
			}

			if (Backend.IsValid())
			{
				UE_LOG(LogPIEAutoRecorder, Display, TEXT("연결 상태: %s (요청 가능=%s)"),
					*Backend->GetStateDescription(),
					Backend->IsReady() ? TEXT("예") : TEXT("아니오"));
			}

			if (Coordinator.IsValid())
			{
				UE_LOG(LogPIEAutoRecorder, Display, TEXT("녹화 상태: %s"), *Coordinator->GetStateDescription());
			}
		})));

	ConsoleCommands.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("PIEAutoRecorder.QueryStatus"),
		TEXT("OBS에 GetRecordStatus를 보냅니다."),
		FConsoleCommandDelegate::CreateLambda([this]()
		{
			if (!CanRunConsoleCommand())
			{
				return;
			}

			Backend->QueryRecordStatus(FGuid::NewGuid(),
				FOnRecordStatusResult::CreateLambda([](bool bSuccess, const FOBSRecordStatus& Status, FGuid SessionId)
				{
					UE_LOG(LogPIEAutoRecorder, Display, TEXT("[결과] GetRecordStatus 성공=%s, outputActive=%s, 길이=%lldms"),
						bSuccess ? TEXT("예") : TEXT("아니오"),
						Status.bOutputActive ? TEXT("true") : TEXT("false"),
						Status.OutputDurationMs);
				}));
		})));

	ConsoleCommands.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("PIEAutoRecorder.StartRecord"),
		TEXT("OBS에 StartRecord를 보냅니다. (3단계 수동 검증용)"),
		FConsoleCommandDelegate::CreateLambda([this]()
		{
			if (!CanRunConsoleCommand())
			{
				return;
			}

			Backend->StartRecording(FGuid::NewGuid(),
				FOnSimpleResult::CreateLambda([](bool bSuccess, const FString& Error, FGuid SessionId)
				{
					UE_LOG(LogPIEAutoRecorder, Display, TEXT("[결과] StartRecord 성공=%s, 비고=%s"),
						bSuccess ? TEXT("예") : TEXT("아니오"),
						Error.IsEmpty() ? TEXT("(없음)") : *Error);
				}));
		})));

	ConsoleCommands.Add(ConsoleManager.RegisterConsoleCommand(
		TEXT("PIEAutoRecorder.StopRecord"),
		TEXT("OBS에 StopRecord를 보내고 파일 경로를 출력합니다. (3단계 수동 검증용)"),
		FConsoleCommandDelegate::CreateLambda([this]()
		{
			if (!CanRunConsoleCommand())
			{
				return;
			}

			Backend->StopRecording(FGuid::NewGuid(),
				FOnStopResult::CreateLambda([](bool bSuccess, const FString& OutputPath, FGuid SessionId)
				{
					UE_LOG(LogPIEAutoRecorder, Display, TEXT("[결과] StopRecord 성공=%s, outputPath=%s"),
						bSuccess ? TEXT("예") : TEXT("아니오"),
						OutputPath.IsEmpty() ? TEXT("(없음)") : *OutputPath);
				}));
		})));
}

// 등록한 콘솔 명령을 전부 해제한다.
void FPIEAutoRecorderModule::UnregisterConsoleCommands()
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	for (IConsoleCommand* Command : ConsoleCommands)
	{
		if (Command)
		{
			ConsoleManager.UnregisterConsoleObject(Command);
		}
	}

	ConsoleCommands.Empty();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPIEAutoRecorderModule, PIEAutoRecorder)
