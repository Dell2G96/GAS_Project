// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "IndicatorLibrary.generated.h"

class AController;
class ULeeIndicatorManagerComponent;
class UObject;
struct FFrame;

UCLASS()
class GAS_PROJECT_API UIndicatorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	
public:
	UIndicatorLibrary();
	
	/**  */
	UFUNCTION(BlueprintCallable, Category = Indicator)
	static ULeeIndicatorManagerComponent* GetIndicatorManagerComponent(AController* Controller);
};
