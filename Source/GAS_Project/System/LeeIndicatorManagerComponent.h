// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "LeeIndicatorManagerComponent.generated.h"


class AController;
class UIndicatorDescriptor;
class UObject;
struct FFrame;

UCLASS(BlueprintType, Blueprintable)
class GAS_PROJECT_API ULeeIndicatorManagerComponent : public UControllerComponent
{
	GENERATED_BODY()

public:

	ULeeIndicatorManagerComponent(const FObjectInitializer& ObjectInitializer);

	static ULeeIndicatorManagerComponent* GetComponent(AController* Controller);


	UFUNCTION(BlueprintCallable, Category= Indicator)
	void AddIndicator(UIndicatorDescriptor* IndicatorDescriptor);

	UFUNCTION(BlueprintCallable, Category= Indicator)
	void RemoveIndicator(UIndicatorDescriptor* IndicatorDescriptor);

	DECLARE_EVENT_OneParam(ULeeIndicatorManagerComponent, FIndicatorEvent,  UIndicatorDescriptor* Indicator);

	FIndicatorEvent OnIndicatorAdded;
	FIndicatorEvent OnIndicatorRemoved;

	const TArray<UIndicatorDescriptor*>& GetIndicators() const {return Indicators;}

private:
	UPROPERTY()
	TArray<TObjectPtr<class UIndicatorDescriptor>> Indicators;
};
