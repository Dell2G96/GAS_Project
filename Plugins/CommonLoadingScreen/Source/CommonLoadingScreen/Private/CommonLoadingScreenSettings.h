// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "CommonLoadingScreenSettings.generated.h"


UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Common Loading Screen"))
class COMMONLOADINGSCREEN_API UCommonLoadingScreenSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	UCommonLoadingScreenSettings();


	// 로딩화면에 사용할 위젯
	// 꼭 확인해야할 경로
	UPROPERTY(Config, EditAnywhere, Category=Display, meta=(MetaClass="/Script/UMG.UserWidget"))
	FSoftClassPath LoadingScreenWidget;

	UPROPERTY(Config, EditAnywhere, Category=Display)
	int32 LoadingScreenZOrder = 10000;

	/* 다른 로딩이 끝난 후 로딩화면을 추가할 시간 초
	 * 텍스처 스트리밍이 불러를 피할 시간을 주기
	 * 보통의 에디터에서는 반복 작업 시간을 위해 이 설정이 적용되지 않지만
	 * HoldLoadingScreenAdditionalSecsEvenInEditor 를 통해 활성화 할 수 있다
	 * */
	 UPROPERTY(Config, EditAnywhere, Category=Configuration, meta=(ForceUnits=s, ConsoleVariable="CommonLoadingScreen.HoldLoadingScreenAdditionalSecs"))
	float HoldLoadingScreenAdditionalSecs = 2.0f;

	// 0이 아니면 로딩화면이 영구적으로 멈춘 것으로 간추하는 시간
	UPROPERTY(Config, EditAnywhere, Category=Configuration, meta=(ForceUnits=s))
	float LoadingScreenHeartbeatHangDuration = 0.0f;

	// 0이 아니면, 로딩 화면을 유지시키는 원인을 기록하는 로그 출력 간격
	UPROPERTY(Config, EditAnywhere, Category=Configuration, meta=(ForceUnits=s))
	float LogLoadingScreenHeartbeatInterval = 5.0f;

	// true 이면, 로딩 화면이 표시되거나 숨겨지는 이유를 매 프레임 로그에 출력
	UPROPERTY(Transient, EditAnywhere, Category=Debugging, meta=(ConsoleVariable="CommonLoadingScreen.LogLoadingScreenReasonEveryFrame"))
	bool LogLoadingScreenReasonEveryFrame = 0;

	// 로딩 화면을 강제로 표시
	UPROPERTY(Transient, EditAnywhere, Category=Debugging, meta=(ConsoleVariable="CommonLoadingScreen.AlwaysShow"))
	bool ForceLoadingScreenVisible = false;


	// 에디터에서도 HoldLoadingScreenAdditionalSecs 지연을 적용할지 여부
	// 로딩 화면을 반복해서 다듬을 떄 사용
	UPROPERTY(Transient, EditAnywhere, Category=Debugging)
	bool HoldLoadingScreenAdditionalSecsEvenInEditor = false;

	// 에디터에서도 HoldLoadingScreenAdditionalSecs 지연을 적용할지 여부
	// 로딩 화면을 반복해서 다듬을 떄 사용
	
	UPROPERTY(config, EditAnywhere, Category=Configuration)
	bool ForceTickLoadingScreenEvenInEditor = true;
};
