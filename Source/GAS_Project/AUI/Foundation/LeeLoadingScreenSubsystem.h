// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "Subsystems/GameInstanceSubsystem.h"
#include "LeeLoadingScreenSubsystem.generated.h"


class UObject;
class UUserWidget;
struct FFrame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoadingScreenWidgetChangedDelegate, TSubclassOf<UUserWidget>, NewWidgetClass);

UCLASS()
class GAS_PROJECT_API ULeeLoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	ULeeLoadingScreenSubsystem();

	UFUNCTION(BlueprintCallable)
	void SetLoadingScreenContentWidget(TSubclassOf<UUserWidget> NewWidgetClass);

	UFUNCTION(BlueprintPure)
	TSubclassOf<UUserWidget> GetLoadingScreenWidgetClass() const;


private:
	UPROPERTY(BlueprintAssignable, meta=(AllowPrivateAccess))
	FLoadingScreenWidgetChangedDelegate OnLoadingScreenWidgetChanged;

	UPROPERTY()
	TSubclassOf<UUserWidget> LoadingScreenWidgetClass;
	
	
};
