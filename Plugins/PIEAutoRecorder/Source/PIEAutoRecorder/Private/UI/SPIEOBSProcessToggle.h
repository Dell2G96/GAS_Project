#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SCheckBox.h"

// 툴바에 표시되는 OBS 체크박스.
// Controller 상태를 화면에 반영하기만 할 뿐, OBS 실행/종료/WebSocket 연결은 직접 수행하지 않는다.
class SPIEOBSProcessToggle : public SCompoundWidget
{
public:
	// IsEnabled는 SWidget::FArguments가 이미 제공하므로 여기서 다시 선언하지 않는다(중복 선언 방지).
	SLATE_BEGIN_ARGS(SPIEOBSProcessToggle) {}
		SLATE_ATTRIBUTE(ECheckBoxState, CheckState)
		SLATE_ATTRIBUTE(FText, Label)
		SLATE_ATTRIBUTE(FText, ToolTip)
		SLATE_EVENT(FOnCheckStateChanged, OnCheckStateChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	ECheckBoxState GetCheckState() const;
	FText GetLabel() const;
	FText GetTooltipText() const;
	bool GetIsEnabled() const;
	void HandleCheckStateChanged(ECheckBoxState NewState);

	TAttribute<ECheckBoxState> CheckStateAttribute;
	TAttribute<FText>          LabelAttribute;
	TAttribute<FText>          ToolTipAttribute;
	TAttribute<bool>           IsEnabledAttribute;
	FOnCheckStateChanged OnCheckStateChangedEvent;
};
