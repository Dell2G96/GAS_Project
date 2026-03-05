// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeExperienceDefinition.h"

#include "GameFeatureAction.h"

ULeeExperienceDefinition::ULeeExperienceDefinition(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void ULeeExperienceDefinition::UpdateAssetBundleData()
{
	Super::UpdateAssetBundleData();

	for (UGameFeatureAction* Action : Actions)
	{
		if (Action)
		{
			Action->AddAdditionalAssetBundleData(AssetBundleData);
		}
	}
	
}
