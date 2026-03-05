// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeActivatableWidget.h"

ULeeActivatableWidget::ULeeActivatableWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

TOptional<FUIInputConfig> ULeeActivatableWidget::GetDesiredInputConfig() const
{
	switch (InputConfig)
	{
	case ELeeWidgetInputMode::GameAndMenu:
		return FUIInputConfig(ECommonInputMode::All, GameMouseCaptureMode);
	case ELeeWidgetInputMode::Game:
		return FUIInputConfig(ECommonInputMode::Game, GameMouseCaptureMode);
	case ELeeWidgetInputMode::Menu:
		return FUIInputConfig(ECommonInputMode::Menu, GameMouseCaptureMode);
	case ELeeWidgetInputMode::Default:
	default:
		return TOptional<FUIInputConfig>();
	}
}
