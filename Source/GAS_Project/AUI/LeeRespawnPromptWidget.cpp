// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeRespawnPromptWidget.h"

#include "GAS_Project/APlayer/LeePlayerController.h"

// 기본 입력 모드를 Menu로 설정 (커서 표시, 게임 입력 차단)
ULeeRespawnPromptWidget::ULeeRespawnPromptWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InputConfig = ELeeWidgetInputMode::Menu;
}

// 서버에 부활 요청 → 중복 클릭 방지를 위해 바로 위젯 비활성화(레이어에서 제거)
void ULeeRespawnPromptWidget::RequestRespawn()
{
	if (ALeePlayerController* PlayerController = GetOwningPlayer<ALeePlayerController>())
	{
		PlayerController->ServerRequestRespawn();
	}

	DeactivateWidget();
}
