// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeBowChargeWidget.h"

#include "Components/ProgressBar.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AAbilitySystem/Abilities/LeeGameplayAbility_BowAim.h"

// 처음엔 숨기고 차지 메시지 구독
void ULeeBowChargeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);

	ListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener(
		MyTags::Souls::Message_Bow_Charge, this, &ThisClass::HandleChargeMessage);
}

// 구독 해제
void ULeeBowChargeWidget::NativeDestruct()
{
	ListenerHandle.Unregister();

	Super::NativeDestruct();
}

// 차지 중이면 비율을 계산해 게이지에 반영
void ULeeBowChargeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bCharging || !GetWorld())
	{
		return;
	}

	const float Ratio = FMath::Clamp((GetWorld()->GetTimeSeconds() - ChargeStartTime) / FMath::Max(MaxChargeTime, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	if (ChargeBar)
	{
		ChargeBar->SetPercent(Ratio);
	}
	OnChargeRatioChanged(Ratio);
}

// 내 캐릭터의 차지 시작/끝만 반영 (리슨 서버에서는 다른 플레이어 메시지가 오지 않지만 안전하게 확인)
void ULeeBowChargeWidget::HandleChargeMessage(FGameplayTag Channel, const FLeeBowChargeMessage& Message)
{
	if (Message.Instigator != GetOwningPlayerPawn())
	{
		return;
	}

	bCharging = Message.bCharging;
	ChargeStartTime = Message.ChargeStartTime;
	MaxChargeTime = Message.MaxChargeTime;

	if (ChargeBar)
	{
		ChargeBar->SetPercent(0.0f);
	}
	SetVisibility(bCharging ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
