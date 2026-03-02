// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeInventoryItemDefinition.h"

ULeeInventoryItemDefinition::ULeeInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

const ULeeInventoryItemFragment* ULeeInventoryItemDefinition::FindFragmentByClass(
	TSubclassOf<ULeeInventoryItemFragment> FragmentClass) const
{
	if (FragmentClass)
	{
		for (ULeeInventoryItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}
	return nullptr;
}
