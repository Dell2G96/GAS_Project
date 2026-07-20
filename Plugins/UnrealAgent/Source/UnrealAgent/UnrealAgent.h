

#pragma once

#include "Modules/ModuleManager.h"

/*
 * 언리얼 에이전트 플러그인 모듈
 * 
 */
class FUnrealAgentModule : public IModuleInterface
{
public:
	using ThisClass = FUnrealAgentModule;
	
public:
	//-----------------------------------------------------------------------------
	// IModuleInterface 오버라이드
	//-----------------------------------------------------------------------------
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	
private:
	//-----------------------------------------------------------------------------
	// Chat UI
	//-----------------------------------------------------------------------------
	
	// 채팅 탭 생성
	TSharedRef<SDockTab> OnSpawnChatTab(const FSpawnTabArgs& SpawnTabArgs);
	
	// 패널 드로어 토클
	void OnToggleChatPannel() const;
	
private:
	// UI 커맨드 리스트
	TSharedPtr<FUICommandList> CommandList;
	
	// 글로벌 입력 프로세서
	TSharedPtr<class FAgentChatInputProcessor> InputProcessor;
	
	// CEF 브라우저 위젯
	TSharedPtr<class SAgentChatBrowser> ChatBrowserWidget;
	
	// 채팅 탭 이름
	static const FName ChatTabName;
};
