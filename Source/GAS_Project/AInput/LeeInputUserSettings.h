// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UserSettings/EnhancedInputUserSettings.h"
#include "PlayerMappableKeySettings.h"

#include "LeeInputUserSettings.generated.h"


/** 
 * Lee 게임의 입력 관련 설정을 위한 사용자 정의 설정 클래스입니다.
 * 이 설정은 Lee Shared Settings와 같은 시점에 직렬화되며,
 * "Serialize" 함수를 통해 클라우드 세이브와도 호환됩니다.
 */
UCLASS()
class GAS_PROJECT_API ULeeInputUserSettings : public UEnhancedInputUserSettings
{
	GENERATED_BODY()
public:
	//~ UEnhancedInputUserSettings 인터페이스 시작
	 virtual void ApplySettings() override;
	//~ UEnhancedInputUserSettings 인터페이스 끝

	// 여기에 추가 입력 설정을 넣을 수 있습니다!
	// 예를 들면:
	// - 게임 내 액션을 위한 "토글 vs 홀드"
	// - 조준 감도
	// - 기타 등등

	// 저장 시 함께 직렬화되도록 프로퍼티에 "SaveGame" 메타데이터를 꼭 지정하세요.
	//UPROPERTY(SaveGame, BlueprintReadWrite, Category="Enhanced Input|User Settings")
	// bool bSomeExampleProperty;
};

/**
 * 플레이어 매퍼블 키 설정은 액션별 키 매핑 단위로 접근 가능한 설정입니다.
 * 여기에는 설정 UI, 입력 트리거,
 * 또는 키 설정 정보를 알아야 하는 다른 곳에서 사용할 수 있는 추가 메타데이터를 넣을 수 있습니다.
 */
UCLASS()
class GAS_PROJECT_API ULeePlayerMappableKeySettings : public UPlayerMappableKeySettings
{
	GENERATED_BODY()
    
public:

	/** 설정 화면에서 이 키에 대해 표시할 툴팁 텍스트를 반환합니다. */
	 const FText& GetTooltipText() const;

protected:
	/** 설정 화면에서 이 액션에 연결될 툴팁입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta=(AllowPrivateAccess=true))
	FText Tooltip = FText::GetEmpty();
};

