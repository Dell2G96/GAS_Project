// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameSettingPressAnyKey.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class GAS_PROJECT_API UGameSettingPressAnyKey : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	UGameSettingPressAnyKey(const FObjectInitializer& Initializer);

	DECLARE_EVENT_OneParam(UGamesegmentPressAnyKey, FOnAnyKeyPressed, FKey /*PressedKey*/);
	FOnAnyKeyPressed OnKeySelected;

	DECLARE_EVENT(UgamesettingPressAnyKey, FOnKeySelectionCanceled);
	FOnKeySelectionCanceled OnKeySelectionCanceled;

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	void HandleKeySelected(FKey InKey);
	void HandleKeySelectinoCanceled();

	void Dismiss(TFunction<void()> PostDismissCallback);

private:
	bool bKeySelected = false;
	TSharedPtr<class FSettingsPressAnyKeyInputPreProcessor> InputProcessor;
};
