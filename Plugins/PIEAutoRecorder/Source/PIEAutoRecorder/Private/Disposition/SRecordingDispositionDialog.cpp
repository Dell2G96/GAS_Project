#include "SRecordingDispositionDialog.h"

#include "PIEAutoRecorderLog.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "HAL/FileManager.h"

#define LOCTEXT_NAMESPACE "RecordingDispositionDialog"

namespace
{
	// 파일 이름에 쓸 수 없는 문자들.
	const TCHAR* InvalidFileNameChars = TEXT("\\/:*?\"<>|");

	// 초를 "3분 12초" 형태로 만든다.
	FString FormatDuration(double Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::RoundToInt(Seconds));
		const int32 Minutes = Total / 60;
		const int32 Rest = Total % 60;

		if (Minutes > 0)
		{
			return FString::Printf(TEXT("%d분 %d초"), Minutes, Rest);
		}

		return FString::Printf(TEXT("%d초"), Rest);
	}

	// 바이트 크기를 MB 단위 문자열로 만든다.
	FString FormatSize(int64 Bytes)
	{
		if (Bytes <= 0)
		{
			return TEXT("확인 불가");
		}

		const double MegaBytes = static_cast<double>(Bytes) / (1024.0 * 1024.0);
		if (MegaBytes >= 1024.0)
		{
			return FString::Printf(TEXT("%.2f GB"), MegaBytes / 1024.0);
		}

		return FString::Printf(TEXT("%.1f MB"), MegaBytes);
	}

	// 정보 한 줄(라벨 + 값)을 만든다.
	TSharedRef<SWidget> MakeInfoRow(const FText& Label, const FString& Value)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 2.0f)
			[
				SNew(SBox)
				.WidthOverride(70.0f)
				[
					SNew(STextBlock).Text(Label)
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(8.0f, 2.0f, 0.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Value))
				.AutoWrapText(true)
			];
	}
}

// 창을 구성한다. 정보 표시 → 입력 → 경고 → 버튼 순서다.
void SRecordingDispositionDialog::Construct(const FArguments& InArgs)
{
	Item = InArgs._Item;
	OnDecided = InArgs._OnDecided;

	// 확장자는 원본 그대로 유지한다. 바꾸면 재생이 깨진다.
	Extension = FPaths::GetExtension(Item.OutputPath);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(12.0f)
		[
			SNew(SVerticalBox)

			// 녹화 정보
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeInfoRow(LOCTEXT("Level", "레벨"), Item.LevelName)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeInfoRow(LOCTEXT("Duration", "길이"), FormatDuration(Item.DurationSeconds))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeInfoRow(LOCTEXT("Size", "크기"), FormatSize(Item.FileSizeBytes))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeInfoRow(LOCTEXT("Source", "원본"), Item.OutputPath)
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f)
			[
				SNew(SSeparator)
			]

			// 파일 이름 (확장자는 회색으로 고정 표시)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(70.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("FileName", "파일 이름"))
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SAssignNew(FileNameBox, SEditableTextBox)
					.Text(FText::FromString(InArgs._DefaultBaseName))
					.OnTextChanged_Lambda([this](const FText&) { ValidateInput(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f, 0.0f, 0.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Extension.IsEmpty() ? FString() : TEXT(".") + Extension))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]

			// 저장 폴더
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(70.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("Directory", "저장 폴더"))
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SAssignNew(DirectoryBox, SEditableTextBox)
					.Text(FText::FromString(InArgs._DefaultDirectory))
					.OnTextChanged_Lambda([this](const FText&) { ValidateInput(); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Browse", "…"))
					.OnClicked(this, &SRecordingDispositionDialog::OnBrowseClicked)
				]
			]

			// 나머지에도 적용
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[
				SNew(SCheckBox)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					bApplyToRest = (NewState == ECheckBoxState::Checked);
				})
				[
					SNew(STextBlock).Text(LOCTEXT("ApplyToRest", "이 선택을 대기 중인 나머지 녹화에도 적용"))
				]
			]

			// 입력 검증 사유 (누른 뒤에 실패하는 것보다 낫다)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(this, &SRecordingDispositionDialog::GetValidationText)
				.ColorAndOpacity(FLinearColor(1.0f, 0.4f, 0.4f))
				.AutoWrapText(true)
			]

			// 경고 문구는 항상 보이게 둔다
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DeleteWarning", "⚠ '저장 안 함'을 누르면 파일이 삭제되며 되돌릴 수 없습니다. 창을 닫으면 원본이 그대로 남습니다."))
				.ColorAndOpacity(FLinearColor(1.0f, 0.75f, 0.3f))
				.AutoWrapText(true)
			]

			// 버튼
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(4.0f, 0.0f))
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("Save", "저장"))
					.HAlign(HAlign_Center)
					.IsEnabled(this, &SRecordingDispositionDialog::IsSaveEnabled)
					.OnClicked(this, &SRecordingDispositionDialog::OnSaveClicked)
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("Discard", "저장 안 함"))
					.HAlign(HAlign_Center)
					.OnClicked(this, &SRecordingDispositionDialog::OnDiscardClicked)
				]
				+ SUniformGridPanel::Slot(2, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("Close", "닫기"))
					.HAlign(HAlign_Center)
					.OnClicked(this, &SRecordingDispositionDialog::OnCloseClicked)
				]
			]
		]
	];

	ValidateInput();
}

// 파일 이름과 폴더가 올바른지 검사한다.
void SRecordingDispositionDialog::ValidateInput()
{
	ValidationMessage = FText::GetEmpty();

	const FString Name = FileNameBox.IsValid() ? FileNameBox->GetText().ToString().TrimStartAndEnd() : FString();
	const FString Dir = DirectoryBox.IsValid() ? DirectoryBox->GetText().ToString().TrimStartAndEnd() : FString();

	if (Name.IsEmpty())
	{
		ValidationMessage = LOCTEXT("EmptyName", "파일 이름을 입력하세요.");
		return;
	}

	FString InvalidChars(InvalidFileNameChars);
	for (int32 Index = 0; Index < InvalidChars.Len(); ++Index)
	{
		if (Name.Contains(InvalidChars.Mid(Index, 1)))
		{
			ValidationMessage = FText::Format(
				LOCTEXT("InvalidChar", "파일 이름에 쓸 수 없는 문자가 있습니다: {0}"),
				FText::FromString(InvalidChars.Mid(Index, 1)));
			return;
		}
	}

	if (Dir.IsEmpty())
	{
		ValidationMessage = LOCTEXT("EmptyDir", "저장 폴더를 입력하세요.");
		return;
	}

	if (!IFileManager::Get().DirectoryExists(*Dir))
	{
		ValidationMessage = LOCTEXT("NoDir", "저장 폴더가 존재하지 않습니다.");
		return;
	}
}

bool SRecordingDispositionDialog::IsSaveEnabled() const
{
	return ValidationMessage.IsEmpty();
}

FText SRecordingDispositionDialog::GetValidationText() const
{
	return ValidationMessage;
}

// 폴더 선택 다이얼로그를 띄운다.
FReply SRecordingDispositionDialog::OnBrowseClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform == nullptr)
	{
		return FReply::Handled();
	}

	const void* ParentHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(AsShared());
	FString Selected;

	if (DesktopPlatform->OpenDirectoryDialog(ParentHandle, TEXT("저장 폴더 선택"),
		DirectoryBox->GetText().ToString(), Selected))
	{
		DirectoryBox->SetText(FText::FromString(Selected));
		ValidateInput();
	}

	return FReply::Handled();
}

FReply SRecordingDispositionDialog::OnSaveClicked()
{
	Decide(ERecordingDecision::Save);
	return FReply::Handled();
}

FReply SRecordingDispositionDialog::OnDiscardClicked()
{
	Decide(ERecordingDecision::Discard);
	return FReply::Handled();
}

// 닫기는 아무것도 하지 않는다. 실수로 파일이 사라지는 경로를 만들지 않는다.
FReply SRecordingDispositionDialog::OnCloseClicked()
{
	Decide(ERecordingDecision::LeaveInPlace);
	return FReply::Handled();
}

// 결정을 대기열에 전달하고 창을 닫는다. 두 번 호출되지 않게 막는다.
void SRecordingDispositionDialog::Decide(ERecordingDecision Decision)
{
	if (bDecided)
	{
		return;
	}
	bDecided = true;

	const FString Directory = DirectoryBox.IsValid() ? DirectoryBox->GetText().ToString().TrimStartAndEnd() : FString();
	const FString BaseName = FileNameBox.IsValid() ? FileNameBox->GetText().ToString().TrimStartAndEnd() : FString();

	OnDecided.ExecuteIfBound(Decision, Directory, BaseName, bApplyToRest);

	if (TSharedPtr<SWindow> Window = OwnerWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
}

#undef LOCTEXT_NAMESPACE
