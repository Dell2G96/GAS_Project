// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeGameViewportClient.h"

#include "CommonUISettings.h"
#include "ICommonUIModule.h"

namespace GameViewportTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Platform_Trait_Input_HardwareCursor, "Platform.Trait.Input.HardwareCursor");

}

ULeeGameViewportClient::ULeeGameViewportClient()
	:Super(FObjectInitializer::Get())
{
}

void ULeeGameViewportClient::Init(struct FWorldContext& WorldContext, UGameInstance* OwningGameInstance,
	bool bCreateNewAudioDevice)
{
	Super::Init(WorldContext, OwningGameInstance, bCreateNewAudioDevice);

	// 프로젝트 설정에서 콘솔/모바일 사용을 위해 소프트웨어 커서를 설정했지만, 
	// 데스크톱에서는 표준 하드웨어 커서를 사용하는 것이 문제없습니다.
	const bool UseHardwareCursor = ICommonUIModule::GetSettings().GetPlatformTraits().HasTag(GameViewportTags::TAG_Platform_Trait_Input_HardwareCursor);
	SetUseSoftwareCursorWidgets(!UseHardwareCursor);
	
}
