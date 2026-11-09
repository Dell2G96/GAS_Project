// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "LeeBowChargeWidget.generated.h"

class UProgressBar;
struct FLeeBowChargeMessage;

/**
 * 활 차지 게이지. 차지 중에만 보이고, 차지 비율(0~1)을 ProgressBar로 표시한다.
 * 조준 어빌리티가 보내는 Souls.Message.Bow.Charge 메시지(시작 시간·최대 시간)를 받아 매 프레임 비율을 계산한다.
 */
UCLASS(Abstract)
class GAS_PROJECT_API ULeeBowChargeWidget : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 차지 비율이 바뀔 때마다 호출 — BP에서 색 변경 등 연출용 (선택) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Lee|Bow")
	void OnChargeRatioChanged(float Ratio);

	/** 차지 게이지 (BP 위젯 트리에 같은 이름의 ProgressBar 필요) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ChargeBar;

private:
	void HandleChargeMessage(FGameplayTag Channel, const FLeeBowChargeMessage& Message);

	FGameplayMessageListenerHandle ListenerHandle;

	bool bCharging = false;
	float ChargeStartTime = 0.0f;
	float MaxChargeTime = 1.0f;
};
