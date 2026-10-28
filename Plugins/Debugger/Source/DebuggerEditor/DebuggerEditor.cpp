#include "DebuggerEditor.h"

#include "FDell2gPluginStyle.h"
#include "ToolBarButton.h"
#include "LevelEditor.h"
#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"

#define LOCTEXT_NAMESPACE "FDebuggerEditorModule"

// 모듈 로드 시 스타일 → 커맨드 → 툴바 확장 → PIE 델리게이트 순으로 등록
void FDebuggerEditorModule::StartupModule()
{
	// Toolbar
	{
		FDell2gPluginStyle::Regist();
		FToolBarButton::Register();

		Extender = MakeShareable(new FExtender());
		Extender->AddToolBarExtension(
			"Play",
			EExtensionHook::After,
			FToolBarButton::Get().Command_Slomo,
			FToolBarExtensionDelegate::CreateRaw(this, &FDebuggerEditorModule::OnAdd_ToolBarButton));

		
		// 레벨 에디터가 창이 생기기 전에 해당 툴바를 추가해야한다
		FLevelEditorModule& levelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		levelEditor.GetToolBarExtensibilityManager()->AddExtender(Extender);

		PIEStartedHandle = FEditorDelegates::PostPIEStarted.AddStatic(&FToolBarButton::OnPIEStarted);
	}
}

// 툴바를 그릴 때 호출되어 Slomo 버튼 4개를 추가
void FDebuggerEditorModule::OnAdd_ToolBarButton(FToolBarBuilder& InBuilders)
{
	TSharedPtr<FDell2gPluginStyle> style = FDell2gPluginStyle::Get();

	InBuilders.AddSeparator();
	InBuilders.AddToolBarButton(FToolBarButton::Get().btnCommandInfo1, NAME_None, FText::FromString("Slow"), FText::FromString(TEXT("배속 -0.1")), style->Slomo_Down);
	InBuilders.AddToolBarButton(FToolBarButton::Get().btnCommandInfo2, NAME_None, FText::FromString("Fast"), FText::FromString(TEXT("배속 +0.1")), style->Slomo_Up);
	InBuilders.AddToolBarButton(FToolBarButton::Get().btnCommandInfo3, NAME_None, FText::FromString("x0.1"), FText::FromString(TEXT("최저 배속")), style->Slomo_0_1);
	InBuilders.AddToolBarButton(FToolBarButton::Get().btnCommandInfo4, NAME_None, FText::FromString("x1.0"), FText::FromString(TEXT("기본 배속")), style->Slomo_1_0);
}

// 모듈 언로드 시 등록 역순으로 해제
void FDebuggerEditorModule::ShutdownModule()
{
	FEditorDelegates::PostPIEStarted.Remove(PIEStartedHandle);

	if (FModuleManager::Get().IsModuleLoaded("LevelEditor") && Extender.IsValid())
	{
		FLevelEditorModule& levelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
		levelEditor.GetToolBarExtensibilityManager()->RemoveExtender(Extender);
	}
	Extender.Reset();

	FToolBarButton::Unregister();
	FDell2gPluginStyle::Unregist();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDebuggerEditorModule, DebuggerEditor)
