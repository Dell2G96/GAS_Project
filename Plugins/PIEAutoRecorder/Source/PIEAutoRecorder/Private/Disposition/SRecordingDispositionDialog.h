#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Disposition/RecordingDispositionQueue.h"

class SEditableTextBox;
class SWindow;

// 사용자가 창에서 내린 결정을 대기열로 돌려주는 콜백.
DECLARE_DELEGATE_FourParams(FOnRecordingDispositionDecided,
	ERecordingDecision /*Decision*/, FString /*Directory*/, FString /*BaseName*/, bool /*bApplyToRest*/);

// PIE 녹화 저장 확인 창.
// 녹화 정보를 보여주고 저장 폴더·파일 이름을 입력받아 저장/삭제/보류 중 하나를 돌려준다.
class SRecordingDispositionDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRecordingDispositionDialog) {}
		SLATE_ARGUMENT(FRecordingDispositionItem, Item)
		SLATE_ARGUMENT(FString, DefaultDirectory)
		SLATE_ARGUMENT(FString, DefaultBaseName)
		SLATE_EVENT(FOnRecordingDispositionDecided, OnDecided)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// 이 위젯을 담은 창. 버튼을 누르면 이 창을 닫는다.
	void SetOwnerWindow(TSharedPtr<SWindow> InWindow) { OwnerWindow = InWindow; }

private:
	// 입력값이 올바른지 검사하고 사유 문구를 갱신한다.
	void ValidateInput();

	bool IsSaveEnabled() const;
	FText GetValidationText() const;

	FReply OnSaveClicked();
	FReply OnDiscardClicked();
	FReply OnCloseClicked();
	FReply OnBrowseClicked();

	// 결정을 한 번만 전달한다. 창을 닫는 과정에서 중복 호출되는 것을 막는다.
	void Decide(ERecordingDecision Decision);

	FRecordingDispositionItem Item;
	FString Extension;

	TSharedPtr<SEditableTextBox> FileNameBox;
	TSharedPtr<SEditableTextBox> DirectoryBox;
	TWeakPtr<SWindow>            OwnerWindow;

	FOnRecordingDispositionDecided OnDecided;

	FText ValidationMessage;
	bool  bApplyToRest = false;
	bool  bDecided = false;
};
