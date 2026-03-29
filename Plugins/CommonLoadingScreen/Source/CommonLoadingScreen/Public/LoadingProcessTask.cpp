// Fill out your copyright notice in the Description page of Project Settings.


#include "LoadingProcessTask.h"

#include "LoadingScreenManager.h"

ULoadingProcessTask* ULoadingProcessTask::CreateLoadingScreenProcessTask(UObject* WorldContextObject,
                                                                         const FString& ShowLoadingScreenReason)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	ULoadingScreenManager* LoadingScreenManager = GameInstance ? GameInstance->GetSubsystem<ULoadingScreenManager>() : nullptr;

	if (LoadingScreenManager)
	{
		ULoadingProcessTask* NewLoadingTask = NewObject<ULoadingProcessTask>(LoadingScreenManager);
		NewLoadingTask->SetShowLoadingScreenReason(ShowLoadingScreenReason);

		LoadingScreenManager->RegisterLoadingProcessor(NewLoadingTask);

		return NewLoadingTask;
	}
	return nullptr;
}

void ULoadingProcessTask::Unregister()
{
	ULoadingScreenManager* LoadingScreenManager = Cast<ULoadingScreenManager>(GetOuter());
	LoadingScreenManager->UnregisterLoadingProcessor(this);
}

void ULoadingProcessTask::SetShowLoadingScreenReason(const FString& InReason)
{
	Reason = InReason;
}

bool ULoadingProcessTask::ShouldShowLoadingScreen(FString& OutReason) const
{
	OutReason = Reason;
	return true;

}
