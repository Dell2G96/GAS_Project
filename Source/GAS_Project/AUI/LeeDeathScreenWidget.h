// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "LeeDeathScreenWidget.generated.h"

struct FLeeVerbMessage;

// "YOU DIED" 화면 위젯 베이스 (W_YouDied)
// 사망/부활 메시지(Souls.Message.Death.*)를 구독해 BP 이벤트로 넘긴다. 연출(텍스트·페이드)은 BP에서 구현
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

private:
	void HandleYouDiedMessage(FGameplayTag Channel, const FLeeVerbMessage& Message);
	void HandleRespawnMessage(FGameplayTag Channel, const FLeeVerbMessage& Message);

	FGameplayMessageListenerHandle YouDiedListenerHandle;
	FGameplayMessageListenerHandle RespawnListenerHandle;
};
