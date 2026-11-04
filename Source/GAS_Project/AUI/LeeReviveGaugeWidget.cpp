// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeReviveGaugeWidget.h"

#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

// 경과 시간을 0으로 돌리고 진행도 0부터 다시 채우기 시작
void ULeeReviveGaugeWidget::StartProgress(float Duration)
{
	TotalDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
	ElapsedTime = 0.0f;
	SetProgress(0.0f);
}

// 매 프레임 경과 시간 / 전체 시간으로 진행도 갱신
void ULeeReviveGaugeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (TotalDuration > 0.0f)
	{
		ElapsedTime += InDeltaTime;
		SetProgress(FMath::Clamp(ElapsedTime / TotalDuration, 0.0f, 1.0f));
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
