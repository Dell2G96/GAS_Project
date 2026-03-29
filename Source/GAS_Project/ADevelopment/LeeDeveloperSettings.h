// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "LeeDeveloperSettings.generated.h"


struct FPropertyChangedEvent;

class ULeeExperienceDefinition;

/** 치트 실행 시점 지정 */
UENUM()
enum class ECheatExecutionTime
{
    /** 치트 매니저가 생성될 때 */
    OnCheatManagerCreated,

    /** 플레이어가 폰을 소유할 때 */
    OnPlayerPawnPossession
};

/** 실행할 치트 설정 */
USTRUCT()
struct FLeeCheatToRun
{
    GENERATED_BODY()

    /** 치트 실행 단계 */
    UPROPERTY(EditAnywhere)
    ECheatExecutionTime Phase = ECheatExecutionTime::OnPlayerPawnPossession;

    /** 실행할 치트 명령어 */
    UPROPERTY(EditAnywhere)
    FString Cheat;
};

/**
 * 개발자 설정 / 에디터 치트
 */
UCLASS(config=EditorPerProjectUserSettings, MinimalAPI)
class ULeeDeveloperSettings : public UDeveloperSettingsBackedByCVars
{
    GENERATED_BODY()

public:
    ULeeDeveloperSettings();

    //~UDeveloperSettings interface
    /** 설정 카테고리 이름을 반환합니다 */
    virtual FName GetCategoryName() const override;
    //~End of UDeveloperSettings interface

public:
    /** 
     * Play in Editor에서 사용할 경험 오버라이드 (설정되지 않으면 열린 맵의 월드 설정의 기본값 사용)
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=Lee, meta=(AllowedTypes="LeeExperienceDefinition"))
    FPrimaryAssetId ExperienceOverride;

    /** 봇 수 오버라이드 활성화 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=LeeBots, meta=(InlineEditConditionToggle))
    bool bOverrideBotCount = false;

    /** 오버라이드 봇 수 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=LeeBots, meta=(EditCondition=bOverrideBotCount))
    int32 OverrideNumPlayerBotsToSpawn = 0;

    /** 플레이어 봇의 공격 허용 여부 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=LeeBots)
    bool bAllowPlayerBotsToAttack = true;

    /** 
     * 에디터에서 플레이할 때 전체 게임 흐름을 테스트할지, 아니면 '플레이어 대기' 등의 게임 단계를 건너뛸지 여부
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=Lee)
    bool bTestFullGameFlowInPIE = false;

    /**
    * 마지막 입력 장치가 게임패드가 아니더라도 포스 피드백 효과를 강제로 재생할지 여부?
    * Lee의 기본 동작은 가장 최근 입력 장치가 게임패드일 때만 포스 피드백을 재생하는 것입니다.
    */
    UPROPERTY(config, EditAnywhere, Category = Lee/*, meta = (ConsoleVariable = "LyraPC.ShouldAlwaysPlayForceFeedback")*/)
    bool bShouldAlwaysPlayForceFeedback = false;

    /** 
     * 에디터에서 게임 로직이 화장품 배경을 로드할지, 반복 속도를 위해 건너뛸지 여부
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=Lee)
    bool bSkipLoadingCosmeticBackgroundsInPIE = false;

    /** 'Play in Editor' 중 자동 실행할 치트 목록 */
    UPROPERTY(config, EditAnywhere, Category=Lee)
    TArray<FLeeCheatToRun> CheatsToRun;
    
    /** 게임플레이 메시지 서브시스템을 통해 브로드캐스트된 메시지를 로그로 기록할지 여부 */
    UPROPERTY(config, EditAnywhere, Category=GameplayMessages, meta=(ConsoleVariable="GameplayMessageSubsystem.LogMessages"))
    bool LogGameplayMessages = false;

#if WITH_EDITORONLY_DATA
    /** 에디터 디툴바를 통해 접근 가능한 일반적인 맵 목록 */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category=Maps, meta=(AllowedClasses="/Script/Engine.World"))
    TArray<FSoftObjectPath> CommonEditorMaps;
#endif
    
#if WITH_EDITOR
public:
    /** 치트가 활성화되어 있을 때 에디터 엔진에서 리마인더 알림을 표시하도록 호출됩니다 */
    void OnPlayInEditorStarted() const;

private:
    /** 설정을 적용합니다 */
    void ApplySettings();
#endif

public:
    //~UObject interface
#if WITH_EDITOR
    /** 속성 변경 후 호출됩니다 */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    /** 설정 재로드 후 호출됩니다 */
    virtual void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;
    /** 속성 초기화 후 호출됩니다 */
    virtual void PostInitProperties() override;
#endif
    //~End of UObject interface
};