// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeInventoryFragment_SetStats.h"

#include "LeeInventoryItemInstance.h"

void ULeeInventoryFragment_SetStats::OnInstanceCreated(class ULeeInventoryItemInstance* Instance) const
{
	for (const auto& InitialItemStat : InitialItemStats)
	{
		Instance->AddStatTagStack(InitialItemStat.Key, InitialItemStat.Value);
	}
}

