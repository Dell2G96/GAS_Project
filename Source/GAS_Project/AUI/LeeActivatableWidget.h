// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "LeeActivatableWidget.generated.h"

UENUM(BlueprintType)
enum class ELeeWidgetInputMode : uint8
{
	Default,
	GameAndMenu,
	Game,
	Menu,
};


UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	ULeeActivatableWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;


#if WITH_EDITOR
	virtual void ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, class IWidgetCompilerLog& CompileLog) const override;
#endif

	
	UPROPERTY(EditDefaultsOnly, Category=Input)
	ELeeWidgetInputMode InputConfig = ELeeWidgetInputMode::Default;

	UPROPERTY(EditDefaultsOnly, Category=Input)
	EMouseCaptureMode GameMouseCaptureMode = EMouseCaptureMode::CapturePermanently;
};
