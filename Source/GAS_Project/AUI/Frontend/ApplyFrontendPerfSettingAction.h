// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "ApplyFrontendPerfSettingAction.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "Use Frontend Perf Settings"))
class GAS_PROJECT_API UApplyFrontendPerfSettingAction : public UGameFeatureAction
{
	GENERATED_BODY()

public:
	//~UGameFeatureAction interface
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	//~End of UGameFeatureAction interface

private:
	static int32 ApplicationCounter;
	
};
