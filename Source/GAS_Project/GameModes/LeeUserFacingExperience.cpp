// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeUserFacingExperience.h"

#include "CommonSessionSubsystem.h"

class UCommonSession_HostSessionRequest* ULeeUserFacingExperience::CreateHostingRequest() const
{
	const FString ExperienceName = ExperienceID.PrimaryAssetName.ToString();
	UCommonSession_HostSessionRequest* Result = NewObject<UCommonSession_HostSessionRequest>();
	Result->MapID = MapID;
	Result->ExtraArgs.Add(TEXT("Experience"), ExperienceName);

	return Result;
}
