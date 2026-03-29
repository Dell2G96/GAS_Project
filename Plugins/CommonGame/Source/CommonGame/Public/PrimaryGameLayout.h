// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CommonUIExtensions.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "UObject/Object.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "PrimaryGameLayout.generated.h"


/**
 * UI의 비동기 로드 작업 상태입니다.
 */
enum class EAsyncWidgetLayerState : uint8
{
	Canceled,           // 취소됨
	Initialize,         // 초기화
	AfterPush           // 푸시 후
};


/**
 * 게임의 기본 UI 레이아웃입니다. 이 위젯 클래스는 단일 플레이어의 모든 UI 레이어를 배치, 푸시, 표시하는 방법을 나타냅니다.
 * 스플릿스크린 게임의 각 플레이어는 각각의 기본 게임 레이아웃을 받습니다.
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class COMMONGAME_API UPrimaryGameLayout : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	static UPrimaryGameLayout* GetPrimaryGameLayoutForPrimaryPlayer(const UObject* WorldContextObject);
	static UPrimaryGameLayout* GetPrimaryGameLayout(APlayerController* PlayerController);
	static UPrimaryGameLayout* GetPrimaryGameLayout(ULocalPlayer* LocalPlayer);

	
	UCommonActivatableWidgetContainerBase* GetLayerWidget(FGameplayTag LayerName);

	template<typename ActivatableWidgetT = UCommonActivatableWidget>
	ActivatableWidgetT* PushWidgetToLayerStack(FGameplayTag LayerName, UClass* ActivatableWidgetClass)
	{
		return PushWidgetToLayerStack<ActivatableWidgetT>(LayerName, ActivatableWidgetClass, [](ActivatableWidgetT&){});
		
	}

	template <typename ActivatableWidgetT = UCommonActivatableWidget>
	TSharedPtr<FStreamableHandle> PushWidgetToLayerStackAsync(FGameplayTag LayerName, bool bSuspendInputUntilComplete, TSoftClassPtr<UCommonActivatableWidget> ActivatableWidgetClass)
	{
		return PushWidgetToLayerStackAsync<ActivatableWidgetT>(LayerName, bSuspendInputUntilComplete, ActivatableWidgetClass, [](EAsyncWidgetLayerState, ActivatableWidgetT*) {});
	}
	
	template <typename ActivatableWidgetT = UCommonActivatableWidget>
	TSharedPtr<FStreamableHandle> PushWidgetToLayerStackAsync(FGameplayTag LayerName, bool bSuspendInputUntilComplete, TSoftClassPtr<UCommonActivatableWidget> ActivatableWidgetClass, TFunction<void(EAsyncWidgetLayerState, ActivatableWidgetT*)> StateFunc)
	{
		static_assert(TIsDerivedFrom<ActivatableWidgetT, UCommonActivatableWidget>::IsDerived, "Only CommonActivatableWidgets can be used here");

		static FName NAME_PushingWidgetToLayer("PushingWidgetToLayer");
		const FName SuspendInputToken = bSuspendInputUntilComplete ? UCommonUIExtensions::SuspendInputForPlayer(GetOwningPlayer(), NAME_PushingWidgetToLayer) : NAME_None;

		FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();
		TSharedPtr<FStreamableHandle> StreamingHandle = StreamableManager.RequestAsyncLoad(ActivatableWidgetClass.ToSoftObjectPath(), FStreamableDelegate::CreateWeakLambda(this,
			[this, LayerName, ActivatableWidgetClass, StateFunc, SuspendInputToken]()
			{
				UCommonUIExtensions::ResumeInputForPlayer(GetOwningPlayer(), SuspendInputToken);

				ActivatableWidgetT* Widget = PushWidgetToLayerStack<ActivatableWidgetT>(LayerName, ActivatableWidgetClass.Get(), [StateFunc](ActivatableWidgetT& WidgetToInit) {
					StateFunc(EAsyncWidgetLayerState::Initialize, &WidgetToInit);
				});

				StateFunc(EAsyncWidgetLayerState::AfterPush, Widget);
			})
		);

		// 이 핸들러가 취소되면 입력을 다시 활성화할 수 있도록 취소 델리게이트를 설정합니다.
		StreamingHandle->BindCancelDelegate(FStreamableDelegate::CreateWeakLambda(this,
			[this, StateFunc, SuspendInputToken]()
			{
				UCommonUIExtensions::ResumeInputForPlayer(GetOwningPlayer(), SuspendInputToken);
				StateFunc(EAsyncWidgetLayerState::Canceled, nullptr);
			})
		);

		return StreamingHandle;
	}

	template<typename ActivatableWidgetT = UCommonActivatableWidget>
	ActivatableWidgetT* PushWidgetToLayerStack(FGameplayTag LayerName, UClass* ActivatableWidgetClass, TFunctionRef<void(ActivatableWidgetT&)> InitInstanceFunc)
	{
		static_assert(TIsDerivedFrom<ActivatableWidgetT, UCommonActivatableWidget> :: IsDerived, "Only CommonActivatableWidgets Can be Used Here");

		if (UCommonActivatableWidgetContainerBase* Layer = GetLayerWidget(LayerName))
		{
			return Layer->AddWidget<ActivatableWidgetT>(ActivatableWidgetClass, InitInstanceFunc);
		}
		return nullptr;
	}

	UFUNCTION(BlueprintCallable, Category="Layer")
	void RegisterLayer(FGameplayTag LayerTag, UCommonActivatableWidgetContainerBase* LayerWidget);

	UPrimaryGameLayout(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(Transient, meta=(Categories = "UI.Layer"))
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>> Layers;
};



























