#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/*
 * CEF 브라우저 위젯
 * SWebBrowser 를 래핑하여 에디터 패널로 사용
 * 에디터 단축키 충돌을 방지, 한국어 입력을 지원
 * 
 */
class SAgentChatBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAgentChatBrowser){}
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& InParentTab);

	//-----------------------------------------------------------------------------
	// SWidget 오버라이드
	//-----------------------------------------------------------------------------

	virtual bool SupportsKeyboardFocus() const override;
	
	// 에이전트 서버 URL을 브라우저에 로드
	void LoadServerUrl() const;
	
	// Slate 종료 전 IME 언바이딩
	void HandleSlatePreShutdown() const;
	
private:
	// CEF 브라우저 위젯
	TSharedPtr<class SWebBrowser> WebBrowserWidget;
	
	// Slate PreShutDown 델리게이트 핸들
	FDelegateHandle PreShutdownHandle;
	
	
	// 에이전트 서버 기본 포트
	static constexpr uint32 DefaultServerPort = 55558;
	
	
};
