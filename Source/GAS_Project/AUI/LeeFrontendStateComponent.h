// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameStateComponent.h"
#include "ControlFlowNode.h"
#include "LoadingProcessInterface.h"

#include "LeeFrontendStateComponent.generated.h"

class FControlFlow;
class FString;
class FText;
class UObject;
struct FFrame;

enum class ECommonUserOnlineContext : uint8;
enum class ECommonUserPrivilege : uint8;
class UCommonActivatableWidget;
class UCommonUserInfo;
class ULyraExperienceDefinition;


UCLASS(Abstract)
class GAS_PROJECT_API ULeeFrontendStateComponent : public UGameStateComponent, public ILoadingProcessInterface
{
	GENERATED_BODY()

public:

	ULeeFrontendStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//UActorComponent
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//UActorComponent

	//iLoadingProcessInterface
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
	//iLoadingProcessInterface

private:
	void OnExperienceLoaded(const class ULeeExperienceDefinition* Experience);

	UFUNCTION()
	void OnUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext);

	void FlowStep_WaitForUserInitialization(FControlFlowNodeRef SubFlow);
	void FlowStep_TryShowPressStartScreen(FControlFlowNodeRef SubFlow);
	void FlowStep_TryJoinRequestedSession(FControlFlowNodeRef SubFlow);
	void FlowStep_TryShowMainScreen(FControlFlowNodeRef SubFlow);

	bool bShouldShowLoadingScreen = true;

	UPROPERTY(EditAnywhere, Category= UI)
	TSoftClassPtr<class UCommonActivatableWidget> PressStartScreenClass;
	
	UPROPERTY(EditAnywhere, Category= UI)
	TSoftClassPtr<UCommonActivatableWidget> MainScreenClass;

	TSharedPtr<FControlFlow> FrontEndFlow;

	FControlFlowNodePtr InProgressPressStartScreen;

	FDelegateHandle OnJoinSessionCompleteEventHandle;

	

	
};
