// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameData.h"

#include "LeeAssetManager.h"

ULeeGameData::ULeeGameData()
{
}

const ULeeGameData& ULeeGameData::Get()
{
	return ULeeAssetManager::Get().GetGameData();
}
