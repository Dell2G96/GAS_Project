// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeReviveGaugeWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GAS_Project/ACharacter/LeeCharacter.h"

// 경과 시간을 0으로 돌리고 진행도 0부터 다시 채우기 시작 (26.11.06 수정 - 넉다운 캐릭터 보관, 시간 텍스트 초기화)
void ULeeReviveGaugeWidget::StartProgress(float Duration, const ALeeCharacter* InDownedCharacter)
{
	TotalDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
	ElapsedTime = 0.0f;
	DownedCharacter = InDownedCharacter;
	LastReviveSeconds = -1;
	LastDeathSeconds = -1;
	SetProgress(0.0f);
	RefreshTimeTexts();
}

// 매 프레임 경과 시간 / 전체 시간으로 진행도 갱신, 남은 시간 텍스트 갱신
void ULeeReviveGaugeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (TotalDuration > 0.0f)
	{
		ElapsedTime += InDeltaTime;
		SetProgress(FMath::Clamp(ElapsedTime / TotalDuration, 0.0f, 1.0f));
		RefreshTimeTexts();
	}
}

// 이미지의 동적 머티리얼 스칼라 파라미터에 진행도 전달
void ULeeReviveGaugeWidget::SetProgress(float Progress)
{
	if (!Image_Gauge)
	{
		return;
	}

	if (UMaterialInstanceDynamic* Material = Image_Gauge->GetDynamicMaterial())
	{
		Material->SetScalarParameterValue(ProgressParameterName, Progress);
	}
}

// 26.11.06 추가 - "부활 N초" / "사망까지 N초 (정지)" 텍스트를 남은 초가 바뀔 때만 갱신
void ULeeReviveGaugeWidget::RefreshTimeTexts()
{
	if (Text_ReviveTime)
	{
		const int32 ReviveSeconds = FMath::Max(0, FMath::CeilToInt(TotalDuration - ElapsedTime));
		if (ReviveSeconds != LastReviveSeconds)
		{
			Text_ReviveTime->SetText(FText::Format(ReviveTimeFormat, ReviveSeconds));
			LastReviveSeconds = ReviveSeconds;
		}
	}

	if (Text_DeathTime)
	{
		const ALeeCharacter* Character = DownedCharacter.Get();
		const int32 DeathSeconds = Character ? FMath::CeilToInt(Character->GetDownedRemainingTime()) : 0;
		const bool bPaused = Character && Character->IsDownedCountdownPaused();
		if (DeathSeconds != LastDeathSeconds || bPaused != bLastDeathPaused)
		{
			Text_DeathTime->SetText(FText::Format(bPaused ? DeathTimePausedFormat : DeathTimeFormat, DeathSeconds));
			LastDeathSeconds = DeathSeconds;
			bLastDeathPaused = bPaused;
		}
	}
}
