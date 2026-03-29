// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LoadingProcessInterface.generated.h"

/**
 * 
 */
UINTERFACE(BlueprintType)
class COMMONLOADINGSCREEN_API ULoadingProcessInterface : public UInterface
{
	GENERATED_BODY()
};

class ILoadingProcessInterface
{
	GENERATED_BODY()

public:
	static bool ShouldShowLoadingScreen(UObject* TestObjet, FString& QutReason);;

	virtual bool ShouldShowLoadingScreen(FString& OutReason) const
	{
		return false;
	}
};


