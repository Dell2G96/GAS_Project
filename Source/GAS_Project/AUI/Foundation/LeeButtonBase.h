// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "UObject/Object.h"
#include "LeeButtonBase.generated.h"


UCLASS(Abstract, BlueprintType, Blueprintable)
class GAS_PROJECT_API ULeeButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void SetButton(const FText& InText);


protected:
	// UUserwidget
	virtual void NativePreConstruct() override;
	// UUserwidget

	// UCommonButtonBase
	virtual void UpdateInputActionWidget() override;
	virtual void OnInputMethodChanged(ECommonInputType CurrentInputType) override;
	// UCommonButtonBase

	void RefreshButtonText();

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateButtonText(const FText& InText);

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateButtonStyle();


private:
	UPROPERTY(EditAnywhere, Category="Button", meta=(InlineEditConditionToggle))
	uint8 bOverride_ButtonText : 1;

	UPROPERTY(EditAnywhere, Category="Button", meta=(editCondition="bOverride_ButtonText"))	FText ButtonText;

	
};
