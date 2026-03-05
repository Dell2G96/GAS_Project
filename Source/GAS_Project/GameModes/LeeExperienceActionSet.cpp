// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeExperienceActionSet.h"

#include "GameFeatureAction.h"

ULeeExperienceActionSet::ULeeExperienceActionSet()
{
}

void ULeeExperienceActionSet::UpdateAssetBundleData()
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
