// Fill out your copyright notice in the Description page of Project Settings.


#include "LeePartyMemberWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"

// 동료 지정 + 이름 표시
void ULeePartyMemberWidget::SetPlayerState(APlayerState* InPlayerState)
{
	PlayerState = InPlayerState;

	if (Text_Name && InPlayerState)
	{
		Text_Name->SetText(FText::FromString(InPlayerState->GetPlayerName()));
	}
}

// 동료 ASC의 복제된 어트리뷰트·태그를 읽어 바와 상태(넉다운 깜빡임, 사망 회색)를 갱신
void ULeePartyMemberWidget::Refresh(float TimeSeconds)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState.Get());
	if (!ASC)
	{
		return;
	}

	const float Health = ASC->GetNumericAttribute(ULeeSoulsStatSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(ULeeSoulsStatSet::GetMaxHealthAttribute());
	const float Stamina = ASC->GetNumericAttribute(ULeeSoulsStatSet::GetStaminaAttribute());
	const float MaxStamina = ASC->GetNumericAttribute(ULeeSoulsStatSet::GetMaxStaminaAttribute());

	if (Bar_Health)
	{
		Bar_Health->SetPercent(MaxHealth > 0.0f ? Health / MaxHealth : 0.0f);
	}
	if (Bar_Stamina)
	{
		Bar_Stamina->SetPercent(MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f);
	}

	// 넉다운 태그는 모든 클라이언트에 복제된다. 사망은 "넉다운이 아닌데 HP 0" 또는 캐릭터가 없을 때로 본다
	const bool bDowned = ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Downed);
	const bool bDead = !bDowned && (Health <= 0.0f || PlayerState->GetPawn() == nullptr);

	// 26.11.04 수정 - 깜빡임 연출은 NativeTick이 매 프레임 담당. 여기서는 상태만 기록하고 표시/숨김만 바꾼다
	bIsDowned = bDowned;
	bIsDead = bDead;

	if (Image_DownedGlow)
	{
		Image_DownedGlow->SetVisibility(bDowned ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (!bDowned)
	{
		SetColorAndOpacity(bDead ? DeadTint : FLinearColor::White);
	}
}

// 26.11.04 추가 - 넉다운 중이면 빨간 발광 이미지와 줄 전체 색을 매 프레임 맥박처럼 깜빡인다
void ULeePartyMemberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsDowned)
	{
		BlinkTime = 0.0f;
		return;
	}

	BlinkTime += InDeltaTime;
	const float Pulse = FMath::Abs(FMath::Sin(BlinkTime * DownedBlinkSpeed));	// 0 ~ 1

	if (Image_DownedGlow)
	{
		FLinearColor GlowColor = DownedGlowColor;
		GlowColor.A = FMath::Lerp(0.15f, 1.0f, Pulse);
		Image_DownedGlow->SetColorAndOpacity(GlowColor);
	}

	SetColorAndOpacity(FMath::Lerp(FLinearColor::White, DownedRowTint, Pulse));
}
