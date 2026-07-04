// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeFinishPromptWidget.h"

#include "GAS_Project/MyTags.h"

void ULeeFinishPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// [주의] WidgetComponent가 위젯을 생성할 때 Outer는 WidgetComponent가 아니라 GameInstance로 설정된다
	// (UUserWidget::CreateWidgetInstance(UWorld&) 참고). 따라서 GetTypedOuter<UWidgetComponent>()로는
	// 절대 오너를 자동 감지할 수 없다 — WidgetComponent 쪽(BP)에서 위젯 생성 직후 SetWatchedEnemy()를
	// 명시적으로 호출해줘야 한다.

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	ListenerHandle = MessageSystem.RegisterListener(
		MyTags::Souls::Message_Finisher_Prompt, this, &ULeeFinishPromptWidget::HandlePromptMessage);
}

void ULeeFinishPromptWidget::NativeDestruct()
{
	ListenerHandle.Unregister();

	Super::NativeDestruct();
}

void ULeeFinishPromptWidget::HandlePromptMessage(FGameplayTag /*Channel*/, const FLeeFinisherPromptMessage& Message)
{
	// 전역 채널이므로, 감시 대상이 지정돼 있다면 그 Enemy가 보낸 메시지만 반응한다
	if (WatchedEnemy.IsValid() && Message.Target != WatchedEnemy.Get())
	{
		return;
	}

	FinishTarget = (Message.Phase == ELeeFinisherPromptPhase::Hidden) ? nullptr : Message.Target;

	OnPromptChanged(Message.Target, Message.Type, Message.Phase);
}
