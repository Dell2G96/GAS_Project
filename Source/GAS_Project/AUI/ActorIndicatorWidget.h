// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IndicatorDescriptor.h"
#include "UObject/Object.h"
#include "ActorIndicatorWidget.generated.h"

class Actor;
class UIndicatorDescriptor;

UINTERFACE()
class GAS_PROJECT_API UIndicatorWidgetInterface : public UInterface
{
	GENERATED_BODY()
public:
	
};

class IIndicatorWidgetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category= "Indicator")
	void BindIndicator(UIndicatorDescriptor* Indicator);
	
	UFUNCTION(BlueprintNativeEvent, Category= "Indicator")
	void UnbindIndicator(const UIndicatorDescriptor* Indicator);
	
};


