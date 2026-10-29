// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeActivatableWidget.h"
#include "LeeRespawnPromptWidget.generated.h"

// "부활하시겠습니까? [예]" 위젯 베이스 (WB_RespawnPrompt)
// Menu 입력 모드로 떠서 마우스 커서를 보여주고, '예' 클릭 시 서버에 부활을 요청한다
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeRespawnPromptWidget : public ULeeActivatableWidget
{
	GENERATED_BODY()

public:
	ULeeRespawnPromptWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// '예' 버튼에서 호출 — 서버에 부활 요청 후 이 위젯을 닫는다
	UFUNCTION(BlueprintCallable, Category = "Lee|Death")
	void RequestRespawn();
};
