// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction_WorldActionBase.h"
#include "Components/GameFrameworkComponentManager.h"

#include "GameFeatureAction_AddInputContextMapping.generated.h"

USTRUCT()
struct FInputMappingContextAndPriority
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Input", meta=(AssetBundles="Client, Server"))
	TSoftObjectPtr<class UInputMappingContext> InputMapping;

	UPROPERTY(EditAnywhere, Category="Input")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, Category="Input")
	bool bRegisterWithSettings = true;
};



UCLASS(meta = (DisplayName = "Add Input Mapping"))
class GAS_PROJECT_API UGameFeatureAction_AddInputContextMapping : public UGameFeatureAction_WorldActionBase
{
	GENERATED_BODY()
public:
	virtual void OnGameFeatureRegistering() override;
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	virtual void OnGameFeatureUnregistering() override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditAnywhere, Category="Input")
	TArray<FInputMappingContextAndPriority> InputMappings;

private:
	struct FPerContextData
	{
		TArray<TSharedPtr<FComponentRequestHandle>> ExtensionRequestHandles;
		TArray<TWeakObjectPtr<class APlayerController>> ControllersAddedTo;
	};

	TMap<FGameFeatureStateChangeContext, FPerContextData> ContextData;

	FDelegateHandle RegisterInputContextMappingsForGameInstanceHandle;

	void RegisterInputMappingContexts();
	void RegisterInputContextMappingForGameInstance(UGameInstance* GameInstance);
	void RegisterInputMappingContextsForLocalPlayer(ULocalPlayer* LocalPlayer);

	void UnRegisterInputMappingContexts();
	void UnRegisterInputContextMappingForGameInstance(UGameInstance* GameInstance);
	void UnRegisterInputMappingContextsForLocalPlayer(ULocalPlayer* LocalPlayer);

	virtual void AddToWorld(const FWorldContext& WorldContext, const struct FGameFeatureStateChangeContext& ChangeContext) override;

	void Reset(FPerContextData& ActiveData);
	void HandleControllerExtension(AActor* Actor, FName EventName, FGameFeatureStateChangeContext ChangeContext);
	void AddInputMappingForPlayer(UPlayer* Player, FPerContextData& ActiveData);
	void RemoveInputMapping(APlayerController* PlayerController, FPerContextData& ActiveData);
	

};
