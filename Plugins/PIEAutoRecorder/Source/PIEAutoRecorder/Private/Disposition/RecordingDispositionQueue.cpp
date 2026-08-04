#include "RecordingDispositionQueue.h"

#include "PIEAutoRecorderLog.h"
#include "PIEAutoRecorderSettings.h"
#include "SRecordingDispositionDialog.h"
#include "PIEAutoRecorderNotification.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SWindow.h"

namespace
{
	// 파일 크기가 안정됐는지 확인하는 폴링 간격(초).
	constexpr float FileReadyPollInterval = 0.5f;

	// 이름 충돌 시 붙일 접미사의 최대 시도 횟수.
	constexpr int32 MaxUniqueSuffix = 999;
}

FRecordingDispositionQueue::FRecordingDispositionQueue()
{
}

// 폴링 ticker를 반드시 해제한다. 남겨 두면 모듈 언로드 후 호출되어 크래시한다.
FRecordingDispositionQueue::~FRecordingDispositionQueue()
{
	StopFileReadyPolling();
}

// 정지 성공한 녹화를 대기열에 넣는다. 창이 떠 있으면 순서를 기다린다.
void FRecordingDispositionQueue::Enqueue(const FRecordingDispositionItem& Item)
{
	if (Item.OutputPath.IsEmpty())
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("경로가 비어 있어 저장 처리를 건너뜁니다."));
		return;
	}

	// 경로는 OBS가 알려준 값만 쓴다. 폴더를 뒤져 "최근 파일"을 찾는 짓은 하지 않는다.
	if (!IFileManager::Get().FileExists(*Item.OutputPath))
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("녹화 파일을 찾을 수 없어 저장 처리를 건너뜁니다: %s"), *Item.OutputPath);
		return;
	}

	Pending.Add(Item);
	UE_LOG(LogPIEAutoRecorder, Log, TEXT("저장 대기열에 추가했습니다. (대기 %d건)"), Pending.Num());

	ProcessNext();
}

// 대기열 선두를 꺼내 파일 준비 상태부터 확인한다.
void FRecordingDispositionQueue::ProcessNext()
{
	// 한 번에 한 건만 처리한다. 창이 10개 동시에 뜨는 일은 없다.
	if (CurrentItem.IsSet() || ActiveWindow.IsValid())
	{
		return;
	}

	if (Pending.Num() == 0)
	{
		return;
	}

	CurrentItem = Pending[0];
	Pending.RemoveAt(0);

	FileReadyStartTime = FPlatformTime::Seconds();
	LastObservedSize = -1;

	StopFileReadyPolling();
	FileReadyTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FRecordingDispositionQueue::TickFileReady), FileReadyPollInterval);
}

// 파일 크기가 두 번 연속 같으면 만질 수 있는 상태로 본다.
bool FRecordingDispositionQueue::TickFileReady(float DeltaTime)
{
	if (!CurrentItem.IsSet())
	{
		FileReadyTickerHandle.Reset();
		return false;
	}

	const FString& Path = CurrentItem->OutputPath;
	const int64 Size = IFileManager::Get().FileSize(*Path);

	if (Size >= 0 && Size == LastObservedSize)
	{
		CurrentItem->FileSizeBytes = Size;
		FileReadyTickerHandle.Reset();
		OnFileReady();
		return false;
	}

	LastObservedSize = Size;

	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	const double Timeout = Settings ? Settings->FileReadyTimeoutSeconds : 5.0;

	if (FPlatformTime::Seconds() - FileReadyStartTime > Timeout)
	{
		// 시간이 초과되면 파일을 건드리지 않는다. 쓰는 중인 파일을 잘라내는 것보다 낫다.
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("녹화 파일이 아직 사용 중입니다. 원본을 그대로 두었습니다: %s"), *Path);
		PIEAutoRecorderNotification::ShowFailure(FString::Printf(TEXT("녹화 파일이 아직 사용 중입니다. 원본을 그대로 두었습니다: %s"), *Path));

		CurrentItem.Reset();
		FileReadyTickerHandle.Reset();
		ProcessNext();
		return false;
	}

	return true;
}

void FRecordingDispositionQueue::StopFileReadyPolling()
{
	if (FileReadyTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(FileReadyTickerHandle);
		FileReadyTickerHandle.Reset();
	}
}

// 파일이 준비됐다. 설정과 이전 선택에 따라 창을 띄우거나 바로 처리한다.
void FRecordingDispositionQueue::OnFileReady()
{
	if (!CurrentItem.IsSet())
	{
		return;
	}

	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	const FRecordingDispositionItem Item = CurrentItem.GetValue();

	const FString DefaultDir = GetDefaultSaveDirectory(Item);
	const FString DefaultName = BuildDefaultFileName(Item);

	// "나머지에도 적용"으로 이미 결정된 경우 창 없이 같은 방식으로 처리한다.
	if (StickyDecision.IsSet())
	{
		const FString Directory = StickyDirectory.IsEmpty() ? DefaultDir : StickyDirectory;
		ApplyDecision(Item, StickyDecision.GetValue(), Directory, DefaultName);
		return;
	}

	// 짧은 녹화 정책을 먼저 본다.
	if (Settings && Settings->MinDurationToPromptSeconds > 0.0f && Item.DurationSeconds < Settings->MinDurationToPromptSeconds)
	{
		switch (Settings->ShortRecordingBehavior)
		{
		case EPIEShortRecordingBehavior::AutoSave:
			UE_LOG(LogPIEAutoRecorder, Log, TEXT("짧은 녹화라 창 없이 저장합니다. (%.1f초)"), Item.DurationSeconds);
			ApplyDecision(Item, ERecordingDecision::Save, DefaultDir, DefaultName);
			return;

		case EPIEShortRecordingBehavior::AutoDelete:
			UE_LOG(LogPIEAutoRecorder, Log, TEXT("짧은 녹화라 창 없이 삭제합니다. (%.1f초)"), Item.DurationSeconds);
			ApplyDecision(Item, ERecordingDecision::Discard, DefaultDir, DefaultName);
			return;

		default:
			break;
		}
	}

	// 창 없이 항상 저장하는 설정.
	if (Settings && Settings->bAutoSaveWithoutPrompt)
	{
		ApplyDecision(Item, ERecordingDecision::Save, DefaultDir, DefaultName);
		return;
	}

	// 창을 띄우지 않는 설정이면 원본을 그대로 둔다.
	if (Settings && !Settings->bPromptOnPIEEnd)
	{
		ApplyDecision(Item, ERecordingDecision::LeaveInPlace, DefaultDir, DefaultName);
		return;
	}

	ShowDialog();
}

// 저장 확인 창을 띄운다. 기본은 비모달이라 다음 PIE가 막히지 않는다.
void FRecordingDispositionQueue::ShowDialog()
{
	if (!CurrentItem.IsSet() || !FSlateApplication::IsInitialized())
	{
		return;
	}

	const FRecordingDispositionItem Item = CurrentItem.GetValue();
	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString(TEXT("PIE 녹화 저장")))
		.ClientSize(FVector2D(560.0f, 400.0f))
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	TSharedRef<SRecordingDispositionDialog> Dialog = SNew(SRecordingDispositionDialog)
		.Item(Item)
		.DefaultDirectory(GetDefaultSaveDirectory(Item))
		.DefaultBaseName(BuildDefaultFileName(Item))
		.OnDecided(FOnRecordingDispositionDecided::CreateLambda(
			[this, Item](ERecordingDecision Decision, FString Directory, FString BaseName, bool bApplyToRest)
			{
				if (bApplyToRest)
				{
					StickyDecision = Decision;
					StickyDirectory = Directory;
				}

				ApplyDecision(Item, Decision, Directory, BaseName);
			}));

	Dialog->SetOwnerWindow(Window);
	Window->SetContent(Dialog);

	// 창이 어떤 이유로 닫혀도 다음 항목이 이어서 처리되도록 한다.
	Window->SetOnWindowClosed(FOnWindowClosed::CreateLambda([this](const TSharedRef<SWindow>&)
	{
		ActiveWindow.Reset();

		// 버튼을 누르지 않고 닫혔다면 원본을 그대로 둔다.
		if (CurrentItem.IsSet())
		{
			UE_LOG(LogPIEAutoRecorder, Log, TEXT("녹화 파일을 원본 위치에 두었습니다: %s"), *CurrentItem->OutputPath);
			CurrentItem.Reset();
		}

		ProcessNext();
	}));

	ActiveWindow = Window;

	if (Settings && Settings->bModalPrompt)
	{
		FSlateApplication::Get().AddModalWindow(Window, nullptr);
	}
	else
	{
		FSlateApplication::Get().AddWindow(Window);
	}
}

void FRecordingDispositionQueue::CloseDialog()
{
	if (TSharedPtr<SWindow> Window = ActiveWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
	ActiveWindow.Reset();
}

// 결정을 실제 파일 조작으로 옮긴다. 실패는 전부 원본 보존으로 수렴한다.
void FRecordingDispositionQueue::ApplyDecision(const FRecordingDispositionItem& Item, ERecordingDecision Decision, const FString& Directory, const FString& BaseName)
{
	// 창 콜백에서 들어온 경우 여기서 현재 항목을 비운다. 창 닫힘 콜백이 중복 처리하지 않게 한다.
	CurrentItem.Reset();

	switch (Decision)
	{
	case ERecordingDecision::Save:
		TrySave(Item, Directory, BaseName);
		break;

	case ERecordingDecision::Discard:
		TryDiscard(Item);
		break;

	default:
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("녹화 파일을 원본 위치에 두었습니다: %s"), *Item.OutputPath);
		break;
	}

	// 창이 아직 열려 있으면 닫고, 아니면 바로 다음 항목으로 간다.
	if (ActiveWindow.IsValid())
	{
		return;
	}

	ProcessNext();
}

// 지정한 폴더로 옮긴다. 덮어쓰지 않고, 실패하면 원본을 보존한다.
bool FRecordingDispositionQueue::TrySave(const FRecordingDispositionItem& Item, const FString& Directory, const FString& BaseName)
{
	const FString Extension = FPaths::GetExtension(Item.OutputPath);
	const FString SafeName = FPaths::MakeValidFileName(BaseName.IsEmpty() ? BuildDefaultFileName(Item) : BaseName);

	if (!IFileManager::Get().DirectoryExists(*Directory))
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("저장 폴더가 없어 원본을 그대로 두었습니다: %s"), *Directory);
		PIEAutoRecorderNotification::ShowFailure(TEXT("저장 폴더가 없어 원본을 그대로 두었습니다."));
		return false;
	}

	const FString Destination = MakeUniqueDestination(Directory, SafeName, Extension);

	// Replace를 false로 두고 자동 증가는 우리가 직접 한다.
	if (IFileManager::Get().Move(*Destination, *Item.OutputPath, /*bReplace=*/false))
	{
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("PIE 녹화를 저장했습니다: %s"), *Destination);
		PIEAutoRecorderNotification::ShowSuccess(FString::Printf(TEXT("PIE 녹화를 저장했습니다: %s"), *Destination));
		return true;
	}

	// 드라이브가 다르면 Move가 실패할 수 있다. 복사 후 원본을 지운다.
	UE_LOG(LogPIEAutoRecorder, Log, TEXT("이동에 실패해 복사 후 삭제로 시도합니다."));

	if (IFileManager::Get().Copy(*Destination, *Item.OutputPath) != COPY_OK)
	{
		// Copy가 실패하면 원본을 절대 지우지 않는다.
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("복사에 실패해 원본을 그대로 두었습니다: %s"), *Item.OutputPath);
		PIEAutoRecorderNotification::ShowFailure(TEXT("저장에 실패해 원본을 그대로 두었습니다."));
		return false;
	}

	if (!IFileManager::Get().Delete(*Item.OutputPath, /*RequireExists=*/true, /*EvenReadOnly=*/false, /*Quiet=*/true))
	{
		// 복사는 됐으나 원본이 남았다. 데이터 손실은 없으므로 경고만 남긴다.
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("복사는 성공했으나 원본을 지우지 못했습니다: %s"), *Item.OutputPath);
	}

	UE_LOG(LogPIEAutoRecorder, Log, TEXT("PIE 녹화를 저장했습니다: %s"), *Destination);
	PIEAutoRecorderNotification::ShowSuccess(FString::Printf(TEXT("PIE 녹화를 저장했습니다: %s"), *Destination));
	return true;
}

// 파일을 삭제한다. 사용자가 명시적으로 선택했을 때만 호출된다.
bool FRecordingDispositionQueue::TryDiscard(const FRecordingDispositionItem& Item)
{
	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();

	if (Settings && Settings->DiscardBehavior == EPIERecordingDiscardBehavior::KeepInPlace)
	{
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("설정에 따라 삭제하지 않고 원본을 유지합니다: %s"), *Item.OutputPath);
		return true;
	}

	// 무엇을 지웠는지 나중에 확인할 수 있어야 한다.
	const int64 Size = IFileManager::Get().FileSize(*Item.OutputPath);
	UE_LOG(LogPIEAutoRecorder, Log, TEXT("녹화 파일을 삭제합니다: %s (%lld 바이트)"), *Item.OutputPath, Size);

	if (!IFileManager::Get().Delete(*Item.OutputPath, /*RequireExists=*/true, /*EvenReadOnly=*/false, /*Quiet=*/false))
	{
		UE_LOG(LogPIEAutoRecorder, Warning, TEXT("삭제에 실패해 원본이 그대로 남았습니다: %s"), *Item.OutputPath);
		PIEAutoRecorderNotification::ShowFailure(TEXT("삭제에 실패해 원본이 그대로 남았습니다."));
		return false;
	}

	PIEAutoRecorderNotification::ShowSuccess(TEXT("녹화 파일을 삭제했습니다."));
	return true;
}

// 이름이 겹치면 _1, _2 … 를 붙여 비어 있는 이름을 찾는다.
FString FRecordingDispositionQueue::MakeUniqueDestination(const FString& Directory, const FString& BaseName, const FString& Extension)
{
	const FString Suffix = Extension.IsEmpty() ? FString() : TEXT(".") + Extension;

	FString Candidate = FPaths::Combine(Directory, BaseName + Suffix);
	if (!IFileManager::Get().FileExists(*Candidate))
	{
		return Candidate;
	}

	for (int32 Index = 1; Index <= MaxUniqueSuffix; ++Index)
	{
		Candidate = FPaths::Combine(Directory, FString::Printf(TEXT("%s_%d%s"), *BaseName, Index, *Suffix));
		if (!IFileManager::Get().FileExists(*Candidate))
		{
			return Candidate;
		}
	}

	// 여기까지 오면 비정상이지만, 마지막 후보를 그대로 돌려주고 Move 실패로 처리되게 둔다.
	return Candidate;
}

// 설정의 SaveFilenameFormat으로 기본 파일명을 만든다. 확장자는 붙이지 않는다.
FString FRecordingDispositionQueue::BuildDefaultFileName(const FRecordingDispositionItem& Item)
{
	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();
	FString Format = Settings ? Settings->SaveFilenameFormat : TEXT("PIE_{level}_{date}_{time}");

	if (Format.IsEmpty())
	{
		Format = TEXT("PIE_{level}_{date}_{time}");
	}

	const FDateTime Now = FDateTime::Now();

	Format = Format.Replace(TEXT("{level}"), *Item.LevelName);
	Format = Format.Replace(TEXT("{date}"), *Now.ToString(TEXT("%y%m%d")));
	Format = Format.Replace(TEXT("{time}"), *Now.ToString(TEXT("%H%M%S")));
	Format = Format.Replace(TEXT("{id}"), *Item.SessionId.ToString(EGuidFormats::Digits).Left(8));

	// 레벨명에 한글·공백·금지 문자가 있어도 안전한 이름이 나와야 한다.
	return FPaths::MakeValidFileName(Format);
}

// 기본 저장 폴더. 설정이 비어 있으면 원본이 있던 폴더를 쓴다.
FString FRecordingDispositionQueue::GetDefaultSaveDirectory(const FRecordingDispositionItem& Item)
{
	const UPIEAutoRecorderSettings* Settings = GetDefault<UPIEAutoRecorderSettings>();

	if (Settings)
	{
		const FString ConfiguredDirectory = Settings->ResolveDefaultSaveDirectory();
		if (!ConfiguredDirectory.IsEmpty())
		{
			return ConfiguredDirectory;
		}
	}

	return FPaths::GetPath(Item.OutputPath);
}

// 에디터 종료 시 미결 항목은 아무 조치 없이 원본을 유지하고 경로만 남긴다.
void FRecordingDispositionQueue::HandleEditorPreExit()
{
	StopFileReadyPolling();
	CloseDialog();

	if (CurrentItem.IsSet())
	{
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("처리하지 못한 녹화 파일: %s"), *CurrentItem->OutputPath);
		CurrentItem.Reset();
	}

	for (const FRecordingDispositionItem& Item : Pending)
	{
		UE_LOG(LogPIEAutoRecorder, Log, TEXT("처리하지 못한 녹화 파일: %s"), *Item.OutputPath);
	}

	Pending.Empty();
}
