#include "ToolBarButton.h"

#include "Engine/Engine.h"
#include "GameFramework/WorldSettings.h"
#include "Styling/AppStyle.h"


FToolBarButton::FToolBarButton() : TCommands<FToolBarButton>("ToolBar_Button", FText(), NAME_None, FAppStyle::GetAppStyleSetName())
{
	Command_Slomo = MakeShareable(new FUICommandList);
}

FToolBarButton::~FToolBarButton()
{
	if (Command_Slomo.IsValid())
		Command_Slomo.Reset();
}

void FToolBarButton::RegisterCommands()
{
#define LOCTEXT_NAMESPACE ""
	UI_COMMAND(btnCommandInfo1,"Slow","",EUserInterfaceActionType::Button,FInputChord());
	UI_COMMAND(btnCommandInfo2,"Fast","",EUserInterfaceActionType::Button,FInputChord());
	UI_COMMAND(btnCommandInfo3,"SlowestSpeed","",EUserInterfaceActionType::Button,FInputChord());
	UI_COMMAND(btnCommandInfo4,"DefaultSpeed","",EUserInterfaceActionType::Button,FInputChord());
#undef LOCTEXT_NAMESPACE
	
	FExecuteAction slow = FExecuteAction::CreateRaw(this, &FToolBarButton::SlowSlomo);
	Command_Slomo->MapAction(btnCommandInfo1, slow);
	
	FExecuteAction fast = FExecuteAction::CreateRaw(this, &FToolBarButton::FastSlomo);
	Command_Slomo->MapAction(btnCommandInfo2,fast);

	FExecuteAction slowest = FExecuteAction::CreateRaw(this, &FToolBarButton::SlowMin);
	Command_Slomo->MapAction(btnCommandInfo3,slowest);

	FExecuteAction defaultAction = FExecuteAction::CreateRaw(this, &FToolBarButton::SlowDefault);
	Command_Slomo->MapAction(btnCommandInfo4,defaultAction);
}

void FToolBarButton::OnPIEStarted(bool bIsSimulating)
{
	float SlomoValue = Get().GameplaySpeed;
	
	UWorld* World = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE)
		{
			World = Context.World();
			break;
		}
	}
	
	if (World)
	{
		World->GetWorldSettings()->SetTimeDilation(SlomoValue);
		UE_LOG(LogTemp, Log, TEXT("PIE Start - Slomo Set: %.2f"), SlomoValue);
	}
}

void FToolBarButton::SlowSlomo()
{
	GameplaySpeed = FMath::Max(GameplaySpeed - 0.1f, 0.1f);
	
	UWorld* World = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
		{
			World = Context.World();
			break;
		}
	}
	if (World)
	{
		World->GetWorldSettings()->SetTimeDilation(GameplaySpeed);
	}
	UE_LOG(LogTemp, Log, TEXT("%.2f"), GameplaySpeed);
}

void FToolBarButton::FastSlomo()
{
	GameplaySpeed = FMath::Min(GameplaySpeed + 0.1f, 2.f);
	
	UWorld* World = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
		{
			World = Context.World();
			break;
		}
	}
	if (World)
	{
		World->GetWorldSettings()->SetTimeDilation(GameplaySpeed);
		
	}
	UE_LOG(LogTemp, Log, TEXT("%.2f"), GameplaySpeed);
}

void FToolBarButton::SlowMin()
{
	GameplaySpeed = 0.1f;
	
	UWorld* World = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
		{
			World = Context.World();
			break;
		}
	}

	if (World)
	{
		World->GetWorldSettings()->SetTimeDilation(GameplaySpeed);
	}
	
	UE_LOG(LogTemp, Log, TEXT("%.2f"), GameplaySpeed);

}

void FToolBarButton::SlowDefault()
{
	GameplaySpeed = 1.f;
	
	UWorld* World = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
		{
			World = Context.World();
			break;
		}
	}

	if (World)
	{
		World->GetWorldSettings()->SetTimeDilation(GameplaySpeed);
	}
	
	UE_LOG(LogTemp, Log, TEXT("%.2f"), GameplaySpeed);
}
