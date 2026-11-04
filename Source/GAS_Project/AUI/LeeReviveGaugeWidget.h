// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LeeReviveGaugeWidget.generated.h"

class UImage;

// 26.11.04 추가 - 부활 채널링 진행도를 원형 게이지로 보여주는 위젯 (W_ReviveGauge)
// 넉다운된 캐릭터 가슴 소켓의 WidgetComponent에 붙으며, 부활시키는 사람의 화면에만 생성된다
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeReviveGaugeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 진행도를 0에서 시작해 Duration초 동안 1까지 채운다
	void StartProgress(float Duration);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 진행도(0~1)를 원형 머티리얼에 반영
	void SetProgress(float Progress);

	// 원형 게이지 이미지. 머티리얼의 스칼라 파라미터(ProgressParameterName)로 채워진 정도를 표시한다
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Gauge;

	// 원형 머티리얼에서 진행도를 받는 스칼라 파라미터 이름
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Revive")
	FName ProgressParameterName = TEXT("Progress");

private:
	float TotalDuration = 0.0f;
	float ElapsedTime = 0.0f;
};
