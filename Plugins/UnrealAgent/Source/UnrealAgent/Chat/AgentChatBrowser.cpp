#include "AgentChatBrowser.h"

#include "SWebBrowser.h"
#include "WebBrowserModule.h"
#include "IWebBrowserSingleton.h"
#include "IWebBrowserWindow.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/ITextInputMethodSystem.h"

void SAgentChatBrowser::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& InParentTab)
{
	//CEF 브라우저 윈도우 생성
	TSharedPtr<IWebBrowserWindow> BrowserWindow;
	if (IWebBrowserSingleton* BrowserSingleton = IWebBrowserModule::Get().GetSingleton())
	{
		FCreateBrowserWindowSettings WindowSettings;
		WindowSettings.BrowserFrameRate = 60;
		
		BrowserWindow = BrowserSingleton->CreateBrowserWindow(WindowSettings);
		
	}
	
	// SWebBrowser 위젯 생성
	SAssignNew(WebBrowserWidget, SWebBrowser, BrowserWindow)
		.ParentWindow(InParentTab->GetParentWindow())
		.ShowControls(false)
		.ShowAddressBar(false)
		.ShowErrorMessage(true)
		.OnBeforePopup_Lambda([](FString Url, FString Frame)-> bool
		{
			// 팝업은 시스템 브라우저로 열고, CEF 내부 팝업은 차단
			FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
			return true;
		})
		.OnLoadCompleted_Lambda([WeakThis = TWeakPtr<SAgentChatBrowser>(SharedThis(this))]()
		{
			// 페이지 로드 완료 후 키보드 포커스를 설정
			if (const TSharedPtr<SAgentChatBrowser> This = WeakThis.Pin())
			{
				if (This->WebBrowserWidget.IsValid() && FSlateApplication::IsInitialized())
				{
					FSlateApplication::Get().SetKeyboardFocus(This->WebBrowserWidget, EFocusCause::SetDirectly);
				}
			}
		});
	
	// IME 바인딩 
	if (FSlateApplication::IsInitialized())
	{
		if (ITextInputMethodSystem* InputMethodSystem = FSlateApplication::Get().GetTextInputMethodSystem())
		{
			WebBrowserWidget->BindInputMethodSystem(InputMethodSystem);
		}
		// Slate 종료 전에 IME 언바인딩 하여 댕글링 포인터를 방지해야한다
		PreShutdownHandle = FSlateApplication::Get().OnPreShutdown().AddSP(this, &SAgentChatBrowser::HandleSlatePreShutdown);
	}
	ChildSlot
	[
		WebBrowserWidget.ToSharedRef()];
	
	// 에이전트 서버 URL 로드
	LoadServerUrl();
}


void SAgentChatBrowser::LoadServerUrl() const
{
	const FString Url = FString::Printf(TEXT("http://localhost:%d"),DefaultServerPort);
	WebBrowserWidget->LoadURL(Url);
	
}


bool SAgentChatBrowser::SupportsKeyboardFocus() const
{
	return true;
}

void SAgentChatBrowser::HandleSlatePreShutdown() const
{
	if (WebBrowserWidget.IsValid())
	{
		WebBrowserWidget->UnbindInputMethodSystem();
	}
}
