// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeUserFacingExperience.h"

#include "CommonSessionSubsystem.h"

class UCommonSession_HostSessionRequest* ULeeUserFacingExperience::CreateHostingRequest(const UObject* WorldContextObject) const
{
	const FString ExperienceName = ExperienceID.PrimaryAssetName.ToString();
	const FString UserFacingExperienceName = GetPrimaryAssetId().PrimaryAssetName.ToString();

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UCommonSession_HostSessionRequest* Result = nullptr;

	if (UCommonSessionSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UCommonSessionSubsystem>() : nullptr)
	{
		Result = Subsystem->CreateOnlineHostSessionRequest();
	}

	if (!Result)
	{
		Result = NewObject<UCommonSession_HostSessionRequest>();
		Result->OnlineMode = ECommonSessionOnlineMode::Online;
		Result->bUseLobbies = true;
		Result->bUseLobbiesVoiceChat = false;
		// 항상 이 세션에 Presence(접속 상태 표시)를 활성화한다.
		// 이 세션은 매치메이킹에 사용되는 기본(primary) 세션이기 때문이다.
		// Presence를 사용하는 온라인 시스템의 경우, 기본 세션에서만 Presence가 활성화되어야 한다.
		Result->bUsePresence = !IsRunningDedicatedServer();
	}
	Result->MapID = MapID;
	Result->ModeNameForAdvertisement = UserFacingExperienceName;
	Result->ExtraArgs = ExtraArgs;
	Result->ExtraArgs.Add(TEXT("Experience"), ExperienceName);
	Result->MaxPlayerCount = MaxPlayerCount;

	// if (ULeeReplaySubsystem::DoesPlatformSupportReplays())
	// {
	// 	if (bRecordReplay)
	// 	{
	// 		Result->ExtraArgs.Add(TEXT("DamageRec"), FString(""));
	// 	}
	// }
	//
	

	return Result;
}
