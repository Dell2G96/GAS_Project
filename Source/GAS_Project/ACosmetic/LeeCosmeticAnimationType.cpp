// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeCosmeticAnimationType.h"

TSubclassOf<class UAnimInstance> FLeeAnimLayerSelectionSet::SelectBestLayer(
	const FGameplayTagContainer& CosmeticTags) const
{
	for (const FLeeAnimLayerSelectionEntry& Rule : LayerRules)
	{
		if ((Rule.Layer != nullptr) && CosmeticTags.HasAll(Rule.RequiredTags))
		{
			return Rule.Layer;
		}
	}
	return DefaultLayer;
}

USkeletalMesh* FLeeAnimBodyStyleSelectionSet::SelectBestBodyStyle(const FGameplayTagContainer& CosmeticTags) const
{
	for (const FLeeAnimBodyStyleSelectionEntry& Rule : MeshRules)
	{
		if ((Rule.Mesh) && CosmeticTags.HasAll(Rule.RequiredTags))
		{
			return Rule.Mesh;
		}
	}

	return DefaultMesh;
}
