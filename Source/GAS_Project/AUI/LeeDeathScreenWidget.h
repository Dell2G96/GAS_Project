// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "LeeDeathScreenWidget.generated.h"

struct FLeeVerbMessage;
class UCommonActivatableWidget;
class UImage;

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

	// 26.11.02 추가 - 보스전에서 보스를 처치했을 때 호출 — BP에서 VICTORY 표시 (잠시 후 서버가 각자 필드로 보낸다)
	UFUNCTION(BlueprintImplementableEvent, Category = "Lee|Death")
	void OnBossVictory();

	// YOU DIED 이후 띄울 부활 질문 위젯 ("부활하시겠습니까? [예]")
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Death")
	TSubclassOf<UCommonActivatableWidget> RespawnPromptClass;

	// YOU DIED 표시 후 부활 질문을 띄우기까지 대기 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Death")
	float RespawnPromptDelay = 2.0f;

	// 26.11.03 추가 - 화면 전체를 어둡게 덮는 배경 (WB_YouDied의 DimBackground). 사망 시 표시, 보스 처치 시 숨김
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> DimBackground;

private:
	// 26.11.03 추가 - 어두운 배경 표시/숨김
	void SetDimBackgroundVisible(bool bVisible);

	void HandleYouDiedMessage(FGameplayTag Channel, const FLeeVerbMessage& Message);
	void HandleRespawnMessage(FGameplayTag Channel, const FLeeVerbMessage& Message);

	// 26.11.02 추가 - 보스전 전멸: YOU DIED만 표시 (부활 질문 없이 서버가 로비로 이동시킨다)
	void HandlePartyWipedMessage(FGameplayTag Channel, const FLeeVerbMessage& Message);

	// 26.11.02 추가 - 보스 처치: VICTORY 표시
	void HandleBossVictoryMessage(FGameplayTag Channel, const FLeeVerbMessage& Message);

	// 부활 질문 위젯을 UI.Layer.Menu에 Push
	void ShowRespawnPrompt();

	// 띄워둔 부활 질문 위젯과 대기 타이머 정리
	void HideRespawnPrompt();

	FGameplayMessageListenerHandle YouDiedListenerHandle;
	FGameplayMessageListenerHandle RespawnListenerHandle;
	FGameplayMessageListenerHandle PartyWipedListenerHandle;
	FGameplayMessageListenerHandle BossVictoryListenerHandle;

	FTimerHandle RespawnPromptTimerHandle;

	TWeakObjectPtr<UCommonActivatableWidget> RespawnPromptWidget;
};
