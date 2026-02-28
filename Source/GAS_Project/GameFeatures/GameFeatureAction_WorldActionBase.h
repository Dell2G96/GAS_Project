// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"


#include "GameFeatureAction_WorldActionBase.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class GAS_PROJECT_API UGameFeatureAction_WorldActionBase : public UGameFeatureAction
{
	GENERATED_BODY()

public:
	virtual void OnGameFeatureActivating(struct FGameFeatureActivatingContext& Context) override;

	/**
	 * interface
	 */
	virtual void AddToWorld(const FWorldContext& WorldContext, const struct FGameFeatureStateChangeContext& ChangeContext) PURE_VIRTUAL(UGameFeatureAction_WorldActionBase::AddToWorld, );
};
