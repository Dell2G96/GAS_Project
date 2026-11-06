#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "ActorIndicatorWidget.h"
#include "LeeInteractionPromptWidget.generated.h"

class UTextBlock;
class UIndicatorDescriptor;

/**
 * 상호작용 프롬프트 위젯 (W_InteractionPrompt의 부모).
 * 인디케이터가 바인딩될 때 Descriptor의 DisplayText(= 상호작용 옵션 문구)를 Text_Prompt에 표시한다.
 */
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeInteractionPromptWidget : public UCommonUserWidget, public IIndicatorWidgetInterface
{
	GENERATED_BODY()

public:
	//~IIndicatorWidgetInterface
	virtual void BindIndicator_Implementation(UIndicatorDescriptor* Indicator) override;
	virtual void UnbindIndicator_Implementation(const UIndicatorDescriptor* Indicator) override;
	//~End of IIndicatorWidgetInterface

protected:
	// 문구를 표시할 텍스트 (BP 디자이너의 텍스트 블록 이름을 Text_Prompt로 맞춘다. CommonTextBlock도 가능)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Prompt;

	// 옵션 문구가 비어 있을 때 표시할 기본 문구
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Interaction")
	FText FallbackText = NSLOCTEXT("Lee", "InteractionPromptFallback", "상호작용");
};
