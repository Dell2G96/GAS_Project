// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction_WorldActionBase.h"
#include "GameplayTagContainer.h"
#include "UIExtensionSystem.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFeatureAction_AddWidget.generated.h"


//-------------------------FLeeHUDLayoutRequest
USTRUCT()
struct FLeeHUDLayoutRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=UI, meta=(AssetBundles="Client"))
	TSoftClassPtr<class UCommonActivatableWidget> LayoutClass;

	UPROPERTY(EditAnywhere, Category=UI)
	FGameplayTag LayerID;
};

// -------------------------FLeeHUDElementEntry

USTRUCT()
struct FLeeHUDElementEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=UI, meta=(AssetBundles="Client"))
	TSoftClassPtr<class UUserWidget> WidgetClass;

	UPROPERTY(EditAnywhere, Category=UI)
	FGameplayTag SlotID;
};

// --------------------------UGameFeatureAction_AddWidget

UCLASS(meta=(DisplayName="Add Widgets"))
class GAS_PROJECT_API UGameFeatureAction_AddWidget : public UGameFeatureAction_WorldActionBase
{
	GENERATED_BODY()
public:
	struct FPerContextData
	{
		TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequests;
		TArray<TWeakObjectPtr<UCommonActivatableWidget>> LayoutsAdded;

		TArray<FUIExtensionHandle> ExtensionHandles;
	};

	void AddWidgets(AActor* Actor, FPerContextData& ActiveData);
	void RemoveWidgets(AActor* Actor, FPerContextData& ActiveData);

	void Reset(FPerContextData& ActiveData);
	void HandleActorExtension(AActor* Actor, FName EventName, FGameFeatureStateChangeContext ChangeContext);

	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;

	virtual void AddToWorld(const FWorldContext& WorldContext, const struct FGameFeatureStateChangeContext& ChangeContext) override;

	TMap<FGameFeatureStateChangeContext, FPerContextData> ContextData;

	UPROPERTY(EditAnywhere, Category=UI)
	TArray<FLeeHUDLayoutRequest> Layout;

	UPROPERTY(EditAnywhere, Category=UI)
	TArray<FLeeHUDElementEntry> Widgets;
};























