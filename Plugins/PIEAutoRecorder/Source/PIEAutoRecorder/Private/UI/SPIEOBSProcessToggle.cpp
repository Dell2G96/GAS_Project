#include "SPIEOBSProcessToggle.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "PIEAutoRecorderToolbar"

// 체크박스와 라벨을 가로로 배치한다. 상태 표시는 전부 바인딩된 Attribute에서 가져온다.
void SPIEOBSProcessToggle::Construct(const FArguments& InArgs)
{
	CheckStateAttribute = InArgs._CheckState;
	LabelAttribute = InArgs._Label;
	ToolTipAttribute = InArgs._ToolTip;
	// IsEnabled는 SWidget::FArguments가 이미 제공하는 속성이다(중복 선언하지 않고 그대로 사용).
	IsEnabledAttribute = InArgs._IsEnabled;
	OnCheckStateChangedEvent = InArgs._OnCheckStateChanged;

	ChildSlot
	[
		SNew(SBox)
		.Padding(FMargin(4.0f, 0.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked(this, &SPIEOBSProcessToggle::GetCheckState)
				.IsEnabled(this, &SPIEOBSProcessToggle::GetIsEnabled)
				.ToolTipText(this, &SPIEOBSProcessToggle::GetTooltipText)
				.OnCheckStateChanged(this, &SPIEOBSProcessToggle::HandleCheckStateChanged)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
			[
				SNew(STextBlock)
				.Text(this, &SPIEOBSProcessToggle::GetLabel)
				.ToolTipText(this, &SPIEOBSProcessToggle::GetTooltipText)
			]
		]
	];
}

ECheckBoxState SPIEOBSProcessToggle::GetCheckState() const
{
	return CheckStateAttribute.Get(ECheckBoxState::Unchecked);
}

FText SPIEOBSProcessToggle::GetLabel() const
{
	return LabelAttribute.Get(LOCTEXT("DefaultLabel", "OBS"));
}

FText SPIEOBSProcessToggle::GetTooltipText() const
{
	return ToolTipAttribute.Get(FText::GetEmpty());
}

bool SPIEOBSProcessToggle::GetIsEnabled() const
{
	return IsEnabledAttribute.Get(true);
}

void SPIEOBSProcessToggle::HandleCheckStateChanged(ECheckBoxState NewState)
{
	OnCheckStateChangedEvent.ExecuteIfBound(NewState);
}

#undef LOCTEXT_NAMESPACE
