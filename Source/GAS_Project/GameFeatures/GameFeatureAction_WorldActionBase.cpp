// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFeatureAction_WorldActionBase.h"

#include "GameFeaturesSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void UGameFeatureAction_WorldActionBase::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (Context.ShouldApplyToWorldContext(WorldContext))
		{
			AddToWorld(WorldContext, Context);
		}
	}
}

