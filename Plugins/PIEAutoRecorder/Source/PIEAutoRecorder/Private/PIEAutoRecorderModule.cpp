#include "PIEAutoRecorderModule.h"

#include "PIEAutoRecorderLog.h"
#include "PIEAutoRecorderSettings.h"
#include "PIEAutoRecorderNotification.h"
#include "OBS/OBSWebSocketBackend.h"
#include "OBS/WindowsOBSProcessPlatform.h"
#include "PIERecordingCoordinator.h"
#include "Disposition/RecordingDispositionQueue.h"
#include "UI/SPIEOBSProcessToggle.h"
#include "Editor.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"

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

	bIsAlive = MakeShared<bool>(true);

	ProcessPlatform = MakeShared<FWindowsOBSProcessPlatform>();
	ProcessController = MakeUnique<FOBSProcessController>(ProcessPlatform.ToSharedRef());
	ProcessController->Initialize();
	ProcessStateChangedHandle = ProcessController->OnStateChanged()
		.AddRaw(this, &FPIEAutoRecorderModule::HandleOBSProcessStateChanged);

	RegisterPIEDelegates();
	RegisterConsoleCommands();

	// Live Coding으로 모듈이 다시 로드돼도 Entry가 한 개만 남도록 Startup Callback을 통해 등록한다.
	ToolbarStartupHandle = UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPIEAutoRecorderModule::RegisterToolbar));

	// 설정 화면에서 값을 바꾸면 즉시 반영한다. 에디터를 다시 켜지 않아도 되도록.
	SettingsChangedHandle = GetMutableDefault<UPIEAutoRecorderSettings>()->OnSettingChanged()
		.AddRaw(this, &FPIEAutoRecorderModule::HandleSettingsChanged);

	// 사전 연결로 PIE 시작 지연을 줄인다. bEnableAutoRecording이 꺼져 있으면 Connect가 스스로 아무것도 하지 않는다.
	if (Settings && Settings->bConnectWhenEditorStarts)
	{
		Backend->Connect();
	}
}

// 모듈 종료. §8.2/§13.3 순서대로 해제한다: UI → Controller → 기존 PIE/Settings Delegate → Backend.
void FPIEAutoRecorderModule::ShutdownModule()
{
	// 파괴된 Module을 향한 콜백이 실행되지 않도록 가장 먼저 표시한다.
	if (bIsAlive.IsValid())
	{
		*bIsAlive = false;
	}

	// 1. 툴바 Owner/Startup Callback 해제. UI가 먼저 사라져야 이후 상태 변경이 UI를 건드리지 않는다.
	UnregisterToolbar();
	if (ToolbarStartupHandle.IsValid())
	{
		UToolMenus::UnRegisterStartupCallback(ToolbarStartupHandle);
		ToolbarStartupHandle.Reset();
	}

	// 2. Controller Shutdown (Ticker/StateChanged Delegate 해제).
	if (ProcessController.IsValid())
	{
		if (ProcessStateChangedHandle.IsValid())
		{
			ProcessController->OnStateChanged().Remove(ProcessStateChangedHandle);
			ProcessStateChangedHandle.Reset();
		}
		ProcessController->Shutdown();
		ProcessController.Reset();
	}
	ProcessPlatform.Reset();

	// 3. 기존 PIE/Settings Delegate 해제.
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

	// 4. Backend Delegate 해제 및 Disconnect.
	if (Backend.IsValid())
	{
		Backend->OnRecordStateChanged().RemoveAll(this);
		Backend->Disconnect();
		Backend.Reset();
	}

	bIsAlive.Reset();

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

	EditorPreExitHandle = FEditorDelegates::OnEditorPreExit.AddLambda([this, Raw, RawQueue]()
	{
		// 소유 녹화 정지를 먼저 시도하고, 그다음 미결 저장 항목을 정리한다(§14.2 Step 1~4).
		Raw->HandleEditorPreExit();

		if (RawQueue)
		{
			RawQueue->HandleEditorPreExit();
		}

		// §14.2 Step 5~7: Backend Disconnect 후 소유 OBS 정상 종료를 시도한다.
		HandleEditorPreExitForOBS();
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

//~ 툴바 OBS 체크박스 ---------------------------------------------------------

// LevelEditor.LevelEditorToolBar.User 슬롯에 체크박스를 등록한다. UToolMenus Startup Callback으로 호출되므로
// Live Coding 재로드 후에도 Owner 단위로 안전하게 다시 등록된다.
void FPIEAutoRecorderModule::RegisterToolbar()
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (ToolMenus == nullptr)
	{
		return;
	}

	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Toolbar = ToolMenus->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.User"));
	if (Toolbar == nullptr)
	{
		return;
	}

	FToolMenuSection& Section = Toolbar->FindOrAddSection(TEXT("PIEAutoRecorder"));

	Section.AddEntry(FToolMenuEntry::InitWidget(
		TEXT("PIEAutoRecorder.OBSProcessToggle"),
		CreateOBSToggleWidget(),
		FText::GetEmpty(),
		/*bNoIndent=*/true,
		/*bSearchable=*/false));
}

// Owner 단위로 등록한 Entry를 전부 해제한다. Reload 후 중복 표시를 막는 핵심이다.
void FPIEAutoRecorderModule::UnregisterToolbar()
{
	if (UToolMenus* ToolMenus = UToolMenus::TryGet())
	{
		ToolMenus->UnregisterOwner(this);
	}
}

// Slate의 (Object, Method) 바인딩 오버로드는 CreateSP를 사용해 TSharedFromThis를 요구한다.
// Module은 UObject도 TSharedFromThis도 아니므로, TAttribute/Delegate를 CreateRaw로 직접 만들어 전달한다.
TSharedRef<SWidget> FPIEAutoRecorderModule::CreateOBSToggleWidget()
{
	return SNew(SPIEOBSProcessToggle)
		.CheckState(TAttribute<ECheckBoxState>::Create(TAttribute<ECheckBoxState>::FGetter::CreateRaw(this, &FPIEAutoRecorderModule::GetOBSCheckState)))
		.Label(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateRaw(this, &FPIEAutoRecorderModule::GetOBSLabel)))
		.ToolTip(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateRaw(this, &FPIEAutoRecorderModule::GetOBSToolTip)))
		.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateRaw(this, &FPIEAutoRecorderModule::GetOBSToggleEnabled)))
		.OnCheckStateChanged(FOnCheckStateChanged::CreateRaw(this, &FPIEAutoRecorderModule::HandleOBSToggleChanged));
}

// §6.1 UI 상태 매핑. RunningExternal은 Undetermined로 표시해 소유하지 않음을 구분한다.
ECheckBoxState FPIEAutoRecorderModule::GetOBSCheckState() const
{
	if (!ProcessController.IsValid())
	{
		return ECheckBoxState::Unchecked;
	}

	switch (ProcessController->GetState())
	{
	case EOBSProcessState::Stopped:
	case EOBSProcessState::Failed:
		return ECheckBoxState::Unchecked;
	case EOBSProcessState::RunningExternal:
		return ECheckBoxState::Undetermined;
	default:
		return ECheckBoxState::Checked;
	}
}

FText FPIEAutoRecorderModule::GetOBSLabel() const
{
	if (!ProcessController.IsValid())
	{
		return FText::FromString(TEXT("OBS"));
	}

	switch (ProcessController->GetState())
	{
	case EOBSProcessState::Starting:          return FText::FromString(TEXT("OBS 시작 중..."));
	case EOBSProcessState::RunningExternal:   return FText::FromString(TEXT("OBS (외부)"));
	case EOBSProcessState::PreparingShutdown: return FText::FromString(TEXT("녹화 정리 중..."));
	case EOBSProcessState::Closing:           return FText::FromString(TEXT("OBS 종료 중..."));
	case EOBSProcessState::Failed:            return FText::FromString(TEXT("OBS 시작 실패"));
	default:                                  return FText::FromString(TEXT("OBS"));
	}
}

// Failed 상태의 실패 사유를 동적으로 보여준다(리뷰 반영).
FText FPIEAutoRecorderModule::GetOBSToolTip() const
{
	if (!ProcessController.IsValid())
	{
		return FText::GetEmpty();
	}

	FString Tooltip = ProcessController->GetStateDescription();

	if (ProcessController->GetState() == EOBSProcessState::RunningOwned && Backend.IsValid())
	{
		Tooltip += FString::Printf(TEXT(" / 연결: %s"), *Backend->GetStateDescription());
	}

	return FText::FromString(Tooltip);
}

bool FPIEAutoRecorderModule::GetOBSToggleEnabled() const
{
	if (!ProcessController.IsValid())
	{
		return false;
	}

	switch (ProcessController->GetState())
	{
	case EOBSProcessState::Starting:
	case EOBSProcessState::PreparingShutdown:
	case EOBSProcessState::Closing:
		return false;
	default:
		return true;
	}
}

// 사용자의 체크/해제 요청을 받는다. 실행/종료는 Controller와 Coordinator에 위임한다.
void FPIEAutoRecorderModule::HandleOBSToggleChanged(ECheckBoxState NewState)
{
	if (!ProcessController.IsValid())
	{
		return;
	}

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("[Toolbar] 사용자 체크/해제 요청. (Checked=%s)"),
		NewState == ECheckBoxState::Checked ? TEXT("true") : TEXT("false"));

	if (NewState == ECheckBoxState::Checked)
	{
		ProcessController->StartOBS();
		return;
	}

	const EOBSProcessState CurrentState = ProcessController->GetState();

	if (CurrentState == EOBSProcessState::RunningExternal)
	{
		PIEAutoRecorderNotification::ShowWarning(TEXT("외부에서 실행한 OBS이므로 종료하지 않습니다."));
		return;
	}

	if (CurrentState == EOBSProcessState::RunningOwned)
	{
		BeginOwnedOBSShutdown();
	}
}

// Controller 상태 변화에 맞춰 WebSocket 연결을 연계한다. 초기 연결 지연은 Backend의 재연결 로직에 위임한다(§9.2).
void FPIEAutoRecorderModule::HandleOBSProcessStateChanged(EOBSProcessState NewState)
{
	if (!Backend.IsValid())
	{
		return;
	}

	switch (NewState)
	{
	case EOBSProcessState::RunningOwned:
	case EOBSProcessState::RunningExternal:
		Backend->Connect();
		break;

	case EOBSProcessState::Stopped:
		Backend->Disconnect();
		break;

	default:
		break;
	}
}

// 체크 해제 흐름 시작. 녹화 정리가 끝나거나 거부될 때까지 Coordinator의 콜백을 기다린다.
void FPIEAutoRecorderModule::BeginOwnedOBSShutdown()
{
	if (!ProcessController.IsValid() || !Coordinator.IsValid())
	{
		return;
	}

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("[OBS Shutdown] 녹화 정리 시작."));
	ProcessController->MarkPreparingShutdown();

	TWeakPtr<bool> WeakAlive = bIsAlive;

	Coordinator->PrepareForOBSShutdown(
		FOnOBSShutdownReady::CreateLambda([this, WeakAlive]()
		{
			if (!WeakAlive.Pin().IsValid())
			{
				return;
			}
			OnRecordingReadyForOBSShutdown();
		}),
		FOnOBSShutdownRejected::CreateLambda([this, WeakAlive](EOBSShutdownBlockReason Reason, FString Message)
		{
			if (!WeakAlive.Pin().IsValid())
			{
				return;
			}
			OnRecordingRejectedForOBSShutdown(Reason, Message);
		}));
}

// 녹화 정리가 끝났다. Backend 연결을 끊고 소유 OBS에 정상 종료를 요청한다.
void FPIEAutoRecorderModule::OnRecordingReadyForOBSShutdown()
{
	if (!ProcessController.IsValid())
	{
		return;
	}

	if (Backend.IsValid())
	{
		Backend->Disconnect();
	}

	ProcessController->RequestCloseOwnedOBS();
}

// 종료 준비가 거부됐다. 체크 상태를 RunningOwned로 복원하고 사용자에게 이유를 알린다(§10.1 에러 복구 매트릭스).
void FPIEAutoRecorderModule::OnRecordingRejectedForOBSShutdown(EOBSShutdownBlockReason Reason, FString Message)
{
	if (ProcessController.IsValid())
	{
		ProcessController->CancelPreparingShutdown();
	}

	UE_LOG(LogPIEAutoRecorder, Warning, TEXT("[OBS Shutdown] 종료 거부: %s"), *Message);
	PIEAutoRecorderNotification::ShowWarning(Message.IsEmpty() ? TEXT("OBS를 종료할 수 없습니다.") : Message);
}

// Editor 종료 §14.2 Step 5~8. Coordinator의 PIE 정리(Step 1~4) 이후에 호출된다.
void FPIEAutoRecorderModule::HandleEditorPreExitForOBS()
{
	if (!ProcessController.IsValid())
	{
		return;
	}

	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	if (Settings == nullptr || !Settings->bCloseOwnedOBSOnEditorExit)
	{
		return;
	}

	if (ProcessController->GetState() != EOBSProcessState::RunningOwned)
	{
		// 외부 OBS는 그대로 둔다. 소유 상태가 아니면 종료할 것이 없다.
		return;
	}

	if (Backend.IsValid())
	{
		Backend->Disconnect();
	}

	ProcessController->MarkPreparingShutdown();
	ProcessController->RequestCloseOwnedOBS();

	// §14.3: 상한 = min(OBSShutdownTimeoutSeconds, 5.0)초. Coordinator 대기와 합쳐도 최대 7.0초를 넘지 않는다.
	const double ShutdownTimeout = FMath::Min(static_cast<double>(Settings->OBSShutdownTimeoutSeconds), 5.0);
	const double StartTime = FPlatformTime::Seconds();

	while (ProcessController->GetState() == EOBSProcessState::Closing
		&& (FPlatformTime::Seconds() - StartTime) < ShutdownTimeout)
	{
		FTSTicker::GetCoreTicker().Tick(0.02f);
		FPlatformProcess::Sleep(0.02f);
	}

	if (ProcessController->GetState() != EOBSProcessState::Stopped)
	{
		UE_LOG(LogPIEAutoRecorder, Warning,
			TEXT("[OBS Process] Editor 종료 중 OBS 종료를 확인하지 못했습니다. 강제 종료하지 않고 계속 진행합니다."));
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPIEAutoRecorderModule, PIEAutoRecorder)
