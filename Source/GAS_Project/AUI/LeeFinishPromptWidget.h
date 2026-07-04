// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GAS_Project/ACharacter/LeeFinisherTargetComponent.h"
#include "LeeFinishPromptWidget.generated.h"

/**
 * "F 처형" / "F 암살" 프롬프트 위젯 베이스 (W_FinishPrompt).
 *
 * GameplayMessageSubsystem의 Souls.Message.Finisher.Prompt 채널을 구독한다.
 * 이 채널은 전역 브로드캐스트라서 Enemy가 여러 마리면 모든 LeeFinisherTargetComponent가
 * 같은 채널로 메시지를 쏜다 — 그래서 이 위젯은 자신이 어느 Enemy를 대표하는지(WatchedEnemy)를
 * 확인해서 그 Enemy가 보낸 메시지만 반응하고 나머지는 무시한다.
 *
 * [주의] WidgetComponent가 위젯을 생성할 때 Outer가 WidgetComponent가 아니라 GameInstance로
 * 잡히기 때문에(엔진 UUserWidget::CreateWidgetInstance 참고), NativeConstruct에서 자동으로
 * 오너를 감지할 방법이 없다 — WatchedEnemy는 **반드시 외부(WidgetComponent를 부착한 BP의
 * BeginPlay 등)에서 SetWatchedEnemy()를 명시적으로 호출**해서 지정해야 한다.
 *
 * BP에서 OnPromptChanged를 구현해 Phase별 표현을 처리한다:
 *  - Hidden     : 숨김
 *  - Visible    : 반투명 표시 (5m 이내, 접근 유도)
 *  - Executable : 강조 표시 (2m 이내, 입력 가능)
 */
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeFinishPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 이 위젯이 대표할 Enemy 액터를 지정. 위젯을 만든 쪽(BP_WidgetComponent의 BeginPlay 등)에서 반드시 명시적으로 호출할 것 */
	UFUNCTION(BlueprintCallable, Category = "Lee|Finish")
	void SetWatchedEnemy(AActor* Enemy) { WatchedEnemy = Enemy; }

protected:
	/** 프롬프트 상태 변화 시 호출 — BP에서 표시/숨김/강조 처리 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lee|Finish")
	void OnPromptChanged(AActor* Target, ELeeFinisherType Type, ELeeFinisherPromptPhase Phase);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lee|Finish")
	AActor* GetFinishTargetActor() const { return FinishTarget.Get(); }

private:
	void HandlePromptMessage(FGameplayTag Channel, const FLeeFinisherPromptMessage& Message);

	FGameplayMessageListenerHandle ListenerHandle;

	/** 이 위젯이 대표하는 Enemy. 설정되어 있으면 이 액터가 보낸 메시지만 반응한다 */
	TWeakObjectPtr<AActor> WatchedEnemy;

	TWeakObjectPtr<AActor> FinishTarget;
};
