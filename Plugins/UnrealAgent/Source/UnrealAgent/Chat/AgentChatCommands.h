#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/*
 * 언리얼 에이전트 채팅 패널의 UI 커맨드
 * 단축키설정하고
 * 모듈 시작 시 Register(), 
 * 종료 시 Unregister() 호출
 */
class FAgentChatCommands : public TCommands<FAgentChatCommands>
{
public:
	FAgentChatCommands();
	
	virtual void RegisterCommands() override;
	
	// ALT + F2 로 패널 드로어를 토클
	TSharedPtr<FUICommandInfo> ToggleChatPanel;
	
};
