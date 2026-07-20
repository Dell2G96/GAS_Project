#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"


class FAgentChatInputProcessor : public IInputProcessor
{
public:
	explicit FAgentChatInputProcessor(const TSharedPtr<FUICommandList>& InCommands);
	
	//-----------------------------------------------------------------------------
	// IInputProcessor 오버라이드
	//-----------------------------------------------------------------------------
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
	
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& KeyEvent) override;
	
private:
	TWeakPtr<FUICommandList> Commands;
	
};
