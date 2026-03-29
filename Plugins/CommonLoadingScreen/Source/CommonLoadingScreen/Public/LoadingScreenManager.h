// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LoadingScreenManager.generated.h"

template<typename InterfaceType> class TScriptInterface;

class FSubsystemCollectionBase;
class IInputProcessor;
class ILoadingProcessInterface;
class SWidget;
class UObject;
class UWorld;
struct FFrame;
struct FWorldContext;

UCLASS()
class COMMONLOADINGSCREEN_API ULoadingScreenManager : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// USubSystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	// USubSystem

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;
	// FTickableGameObject

	UFUNCTION(BlueprintCallable, Category=LoadingScreen)
	FString GetDebugReasonForShowingOrHidingLoadingScreen() const
	{
		return DebugReasonForShowingOrHidingLoadingScreen;
	}

	/** 로딩 화면이 현재 표시 중이면 True를 반환합니다 */
	bool GetLoadingScreenDisplayStatus() const
	{
		return bCurrentlyShowingLoadingScreen;
	}

	/** 로딩 화면의 표시 상태가 바뀔 때 호출됩니다 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoadingScreenVisibilityChangedDelegate, bool);
	FORCEINLINE FOnLoadingScreenVisibilityChangedDelegate& OnLoadingScreenVisibilityChangedDelegate() { return LoadingScreenVisibilityChanged; }

	void RegisterLoadingProcessor(TScriptInterface<ILoadingProcessInterface> Interface);
	void UnregisterLoadingProcessor(TScriptInterface<ILoadingProcessInterface> Interface);

private:
     void HandlePreLoadMap(const FWorldContext& WorldContext, const FString& MapName);
     void HandlePostLoadMap(UWorld* World);

    /** 로딩 화면을 보여줄지 숨길지를 결정합니다. 매 프레임 호출됩니다. */
     void UpdateLoadingScreen();

    /** 로딩 화면을 보여줘야 하면 true를 반환합니다. */
     bool CheckForAnyNeedToShowLoadingScreen();

    /** 로딩 화면을 보여주고 싶으면 true를 반환합니다.  
     * (보여줘야 하거나, 다른 이유로 강제로 켜져 있는 경우 포함) */
     bool ShouldShowLoadingScreen();

    /** 이 화면을 사용하기 전에 초기 로딩 흐름 중인지 반환합니다 */
     bool IsShowingInitialLoadingScreen() const;

    /** 로딩 화면을 표시합니다. 뷰포트에 로딩 화면 위젯을 설정합니다 */
     void ShowLoadingScreen();

    /** 로딩 화면을 숨깁니다. 로딩 화면 위젯이 제거됩니다 */
     void HideLoadingScreen();

    /** 위젯을 뷰포트에서 제거합니다 */
     void RemoveWidgetFromViewport();

    /** 로딩 화면이 보이는 동안 게임 내 입력이 사용되지 않도록 막습니다 */
     void StartBlockingInput();

    /** 입력을 막고 있었다면 게임 내 입력을 다시 허용합니다 */
     void StopBlockingInput();

     void ChangePerformanceSettings(bool bEnabingLoadingScreen);

private:
    /** 로딩 화면 표시 상태가 변경될 때 방송되는 델리게이트 */
    FOnLoadingScreenVisibilityChangedDelegate LoadingScreenVisibilityChanged;

    /** 표시 중인 로딩 화면 위젯에 대한 참조(있다면) */
    TSharedPtr<SWidget> LoadingScreenWidget;

    /** 로딩 화면이 보이는 동안 모든 입력을 처리해 버리는 입력 전처리기 */
    TSharedPtr<IInputProcessor> InputPreProcessor;

    /** 외부 로딩 처리기들. 컴포넌트일 수도 있고 로딩을 지연시키는 액터일 수도 있습니다 */
    TArray<TWeakInterfacePtr<ILoadingProcessInterface>> ExternalLoadingProcessors;

    /** 로딩 화면이 올라와 있는 이유 */
    FString DebugReasonForShowingOrHidingLoadingScreen;

    /** 로딩 화면을 표시하기 시작한 시간 */
    double TimeLoadingScreenShown = 0.0;

    /** 로딩 화면이 최근에 숨겨지길 원했던 시간(최소 표시 시간 때문에 아직 남아 있을 수 있음) **/
    double TimeLoadingScreenLastDismissed = -1.0;

    /** 로딩 화면이 아직 떠 있는 이유를 다음에 로그할 시간까지 남은 초 */
    double TimeUntilNextLogHeartbeatSeconds = 0.0;

    /** PreLoadMap과 PostLoadMap 사이인지 여부 */
    bool bCurrentlyInLoadMap = false;

    /** 로딩 화면이 현재 표시 중인지 여부 */
    bool bCurrentlyShowingLoadingScreen = false;
};



















