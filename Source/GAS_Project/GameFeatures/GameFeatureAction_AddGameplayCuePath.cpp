// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFeatureAction_AddGameplayCuePath.h"

UGameFeatureAction_AddGameplayCuePath::UGameFeatureAction_AddGameplayCuePath() : Super()
{
	DirectoryPathsToAdd.Add(FDirectoryPath{TEXT("/GameplayCues")});
}
