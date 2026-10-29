// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "LeeDeathScreenWidget.generated.h"

struct FLeeVerbMessage;
class UCommonActivatableWidget;

// "YOU DIED" 화면 위젯 베이스 (W_YouDied)
// 사망/부활 메시지(Souls.Message.Death.*)를 구독해 BP 이벤트로 넘기고, 사망 후 부활 질문 위젯을 Menu 레이어에 띄운다
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeDeathScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	// 플레이어가 사망했을 때 호출 — BP에서 YOU DIED 표시
	UFUNCTION(BlueprintImplementableEvent, Category = "Lee|Death")
	void OnYouDied();

	// 부활 직전에 호출 — BP에서 화면 숨김
	UFUNCTION(BlueprintImplementableEvent, Category = "Lee|Death")
	void OnRespawn();

	// YOU DIED 이후 띄울 부활 질문 위젯 ("부활하시겠습니까? [예]")
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Death")
	TSubclassOf<UCommonActivatableWidget> RespawnPromptClass;

	// YOU DIED 표시 후 부활 질문을 띄우기까지 대기 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Death")
	float RespawnPromptDelay = 2.0f;

private:
	void HandleYouDiedMessage(FGameplayTag Channel, const FLeeVerbMessage& Message);
	void HandleRespawnMessage(FGameplayTag Channel, const FLeeVerbMessage& Message);

	// 부활 질문 위젯을 UI.Layer.Menu에 Push
	void ShowRespawnPrompt();

	// 띄워둔 부활 질문 위젯과 대기 타이머 정리
	void HideRespawnPrompt();

	FGameplayMessageListenerHandle YouDiedListenerHandle;
	FGameplayMessageListenerHandle RespawnListenerHandle;

	FTimerHandle RespawnPromptTimerHandle;

	TWeakObjectPtr<UCommonActivatableWidget> RespawnPromptWidget;
};
