// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeLoadingScreenSubsystem.h"

#include "Blueprint/UserWidget.h"


ULeeLoadingScreenSubsystem::ULeeLoadingScreenSubsystem()
{
}

void ULeeLoadingScreenSubsystem::SetLoadingScreenContentWidget(TSubclassOf<UUserWidget> NewWidgetClass)
{
	if (LoadingScreenWidgetClass != NewWidgetClass)
	{
		LoadingScreenWidgetClass = NewWidgetClass;
		
		OnLoadingScreenWidgetChanged.Broadcast(LoadingScreenWidgetClass);
	}
}

TSubclassOf<UUserWidget> ULeeLoadingScreenSubsystem::GetLoadingScreenWidgetClass() const
{
	return LoadingScreenWidgetClass;
}
