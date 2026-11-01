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

const FName FUnrealAgentModule::ChatTabName(TEXT("UnrealAgentChat"));

//-----------------------------------------------------------------------------
// IModuleInterface 오버라이드
//-----------------------------------------------------------------------------

void FUnrealAgentModule::StartupModule()
{
	// 1. 커맨드 등록 (Alt+F2 단축키 정의)
	FAgentChatCommands::Register();
	
	// 2. 커맨드 리스트 생성 및 액션 바인딩
	CommandList = MakeShareable(new FUICommandList);
	
	CommandList->MapAction(
		FAgentChatCommands::Get().ToggleChatPanel,
		FExecuteAction::CreateRaw(this, &ThisClass::OnToggleChatPanel));
	
	// 3. 탭 스포너 등록
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		ChatTabName,
		FOnSpawnTab::CreateRaw(this, &ThisClass::OnSpawnChatTab))
		.SetDisplayName(INVTEXT("Dellg-AI Agent"))	// UE 5.6은 일반 탭이라 이름이 보이므로 지정
		.SetMenuType(ETabSpawnerMenuType::Hidden)
		.SetCanSidebarTab(false);
	
	// 4. 글로벌 입력 프로세서 등록 (포커스 위치와 무관하게 단축키 작동)
	InputProcessor = MakeShared<FAgentChatInputProcessor>(CommandList);
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
	}
}

void FUnrealAgentModule::ShutdownModule()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}
	InputProcessor.Reset();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ChatTabName);

	FAgentChatCommands::Unregister();
}

//-----------------------------------------------------------------------------
// Chat UI
//-----------------------------------------------------------------------------

TSharedRef<SDockTab> FUnrealAgentModule::OnSpawnChatTab(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(INVTEXT("Dellg-AI Agent"))
		.OnTabClosed_Lambda([this](TSharedRef<SDockTab>)
		{
			ChatBrowserWidget.Reset();
		});

	ChatBrowserWidget = SNew(SAgentChatBrowser, DockTab)
		.Clipping(EWidgetClipping::ClipToBounds);

	DockTab->SetContent(ChatBrowserWidget.ToSharedRef());

	return DockTab;
}

void FUnrealAgentModule::OnToggleChatPanel() const
{
	// UE 5.6에는 패널 드로어(TryToggleTabInPanelDrawer)가 없습니다 (5.7에서 추가됨)
	// 대신 일반 도킹 탭을 열려 있으면 닫고, 닫혀 있으면 엽니다
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

