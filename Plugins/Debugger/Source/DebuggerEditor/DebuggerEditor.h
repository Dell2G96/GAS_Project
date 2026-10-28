#pragma once

#include "Modules/ModuleManager.h"

class FExtender;
class FToolBarBuilder;

// 에디터 모듈: 아이콘 스타일·툴바 커맨드를 등록하고 레벨 에디터 Play 툴바 옆에 Slomo 버튼을 붙임
class FDebuggerEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<FExtender> Extender;
	FDelegateHandle PIEStartedHandle;

public:
	void OnAdd_ToolBarButton(FToolBarBuilder& InBuilders);
};
