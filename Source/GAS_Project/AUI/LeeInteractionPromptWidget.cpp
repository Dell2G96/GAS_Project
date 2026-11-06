#include "LeeInteractionPromptWidget.h"

#include "Components/TextBlock.h"
#include "IndicatorDescriptor.h"

// 인디케이터가 붙을 때 옵션 문구를 표시 (비어 있으면 기본 문구)
void ULeeInteractionPromptWidget::BindIndicator_Implementation(UIndicatorDescriptor* Indicator)
{
	if (!Text_Prompt)
	{
		return;
	}

	const FText& DisplayText = Indicator ? Indicator->GetDisplayText() : FText::GetEmpty();
	Text_Prompt->SetText(DisplayText.IsEmpty() ? FallbackText : DisplayText);
}

// 인디케이터가 떨어질 때 (위젯 풀 재사용 대비 문구 비우기)
void ULeeInteractionPromptWidget::UnbindIndicator_Implementation(const UIndicatorDescriptor* Indicator)
{
	if (Text_Prompt)
	{
		Text_Prompt->SetText(FText::GetEmpty());
	}
}
