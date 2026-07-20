// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealAgent.h"

#include "Chat/AgentChatCommands.h"
#include "Chat/AgentChatInputProcessor.h"
#include "Chat/AgentChatBrowser.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Containers/Ticker.h"
#include "StatusBarSubsystem.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "FUnrealAgentModule"

const FName FUnrealAgentModule::ChatTabName(TEXT("AgentChatTab"));


//-----------------------------------------------------------------------------
// IModuleInterface 오버라이드
//-----------------------------------------------------------------------------
void FUnrealAgentModule::StartupModule()
{
	// 1. 커맨드 등록
	FAgentChatCommands::Register();
	
	// 2. 커맨드 리스트 생성 및 바인딩
	CommandList = MakeShareable(new FUICommandList);
	
	CommandList->MapAction(
		FAgentChatCommands::Get().ToggleChatPanel, 
		FExecuteAction::CreateRaw(this, &ThisClass::OnToggleChatPannel));
	
	// 3. 탭 스포너 등록
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
	ChatTabName,
	FOnSpawnTab::CreateRaw(this, &ThisClass::OnSpawnChatTab))
	.SetDisplayName(FText::GetEmpty())
	.SetMenuType(ETabSpawnerMenuType::Hidden)
	.SetCanSidebarTab(false);
	
	// 4. 글로벌 입력 프로세서 등록
	InputProcessor = MakeShared<FAgentChatInputProcessor>(CommandList);
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
	}
}

void FUnrealAgentModule::ShutdownModule()
{
	// 입력 프로세서 해제
	if (FSlateApplication::IsInitialized() && InputProcessor.IsValid())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}
	InputProcessor.Reset();
	
	// 탭 스포너 해제
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ChatTabName);
	}
	
	// 커맨드 해제
	FAgentChatCommands::Unregister();
}


//-----------------------------------------------------------------------------
// Chat UI
//-----------------------------------------------------------------------------
TSharedRef<SDockTab> FUnrealAgentModule::OnSpawnChatTab(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.TabRole(NomadTab)
		.Label(INVTEXT(" "))
		.OnTabClosed_Lambda([this](TSharedPtr<SDockTab>)
		{
			ChatBrowserWidget.Reset();
		});
	
	ChatBrowserWidget = SNew(SAgentChatBrowser, DockTab).Clipping(EWidgetClipping::ClipToBounds);
	
	DockTab->SetContent(ChatBrowserWidget.ToSharedRef());
	
	return DockTab;
}

void FUnrealAgentModule::OnToggleChatPannel() const
{
	#pragma region 5.7버전 API 함수

	
	// // 커서 위치의 윈도우에서 패널 드로우를 토클
	// TSharedPtr<FTabManager> TabManager;
	//
	// const TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
	// if (FocusedWidget.IsValid())
	// {
	// 	FWidgetPath WidgetPath;
	// 	FSlateApplication::Get().FindPathToWidget(FocusedWidget.ToSharedRef(), WidgetPath);
	// 	
	// 	if (WidgetPath.IsValid())
	// 	{
	// 		TabManager = FGlobalTabmanager::Get()->GetSubTabManagerForWindow(WidgetPath.GetWindow());
	// 	}
	// }
	//
	// if (TabManager.IsValid())
	// {
	// 	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	// 	TabManager->TryToggleTabInPanelDrawer(ChatTabName, {});
	// 	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	// }
#pragma endregion 
	
	//5.6 버전 일반 도킹 탭
	const TSharedPtr<SDockTab> ExistingTab = FGlobalTabmanager::Get()->FindExistingLiveTab(ChatTabName);
	if (ExistingTab.IsValid())
	{
		ExistingTab->RequestCloseTab();
	}
	else
	{
		FGlobalTabmanager::Get()->TryInvokeTab(ChatTabName);
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealAgentModule, UnrealAgent)