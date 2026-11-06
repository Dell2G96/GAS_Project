// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LeeReviveGaugeWidget.generated.h"

class UImage;
class UTextBlock;
class ALeeCharacter;

// 26.11.04 추가 - 부활 채널링 진행도를 원형 게이지로 보여주는 위젯 (W_ReviveGauge)
// 넉다운된 캐릭터 가슴 소켓의 WidgetComponent에 붙으며, 부활시키는 사람의 화면에만 생성된다
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeReviveGaugeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 진행도를 0에서 시작해 Duration초 동안 1까지 채운다
	// 26.11.06 수정 - 넉다운된 캐릭터를 받아 "사망까지 남은 시간"도 함께 표시
	void StartProgress(float Duration, const ALeeCharacter* InDownedCharacter);

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

	// 26.11.06 추가 - 부활 완료까지 남은 초 (BP에 같은 이름의 TextBlock이 있으면 표시, 없으면 생략)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_ReviveTime;

	// 26.11.06 추가 - 사망까지 남은 초 (부활 중에는 멈춰 있음)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DeathTime;

	// 26.11.06 추가 - 표시 문구 형식 ({0} = 초)
	UPROPERTY(EditDefaultsOnly, Category = "Lee|Revive")
	FText ReviveTimeFormat = NSLOCTEXT("Lee", "ReviveGaugeReviveTime", "부활 {0}초");

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Revive")
	FText DeathTimeFormat = NSLOCTEXT("Lee", "ReviveGaugeDeathTime", "사망까지 {0}초");

	UPROPERTY(EditDefaultsOnly, Category = "Lee|Revive")
	FText DeathTimePausedFormat = NSLOCTEXT("Lee", "ReviveGaugeDeathTimePaused", "사망까지 {0}초 (정지)");

private:
	// 26.11.06 추가 - 남은 초가 바뀔 때만 텍스트를 갱신
	void RefreshTimeTexts();

	float TotalDuration = 0.0f;
	float ElapsedTime = 0.0f;

	TWeakObjectPtr<const ALeeCharacter> DownedCharacter;
	int32 LastReviveSeconds = -1;
	int32 LastDeathSeconds = -1;
	bool bLastDeathPaused = false;
};
