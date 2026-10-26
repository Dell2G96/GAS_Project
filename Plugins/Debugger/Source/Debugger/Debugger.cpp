// Copyright Epic Games, Inc. All Rights Reserved.

#include "Debugger.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "DebuggerCategory.h"
#endif

#define LOCTEXT_NAMESPACE "FDebuggerModule"

// 모듈 로드 시 Gameplay Debugger에 "Debugger" 카테고리 등록
void FDebuggerModule::StartupModule()
{
	// Debugger
#if WITH_GAMEPLAY_DEBUGGER
	{
		IGameplayDebugger::FOnGetCategory category;
		category.BindStatic(&FDebugger::MakeInstance);

		IGameplayDebugger::Get().RegisterCategory("Debugger", category, EGameplayDebuggerCategoryState::EnabledInGameAndSimulate, 6);
		IGameplayDebugger::Get().NotifyCategoriesChanged();
	}
#endif
}

// 모듈 언로드 시 등록했던 카테고리 해제
void FDebuggerModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER
	if (IGameplayDebugger::IsAvailable())
	{
		IGameplayDebugger::Get().UnregisterCategory("Debugger");
		IGameplayDebugger::Get().NotifyCategoriesChanged();
	}
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDebuggerModule, Debugger)
