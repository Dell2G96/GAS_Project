// Fill out your copyright notice in the Description page of Project Settings.


#include "ReviveWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/TextBlock.h"
//
// void UReviveWidget::NativeConstruct()
// {
// 	Super::NativeConstruct();
// 	CooldownCounterText->SetVisibility(ESlateVisibility::Hidden);
// 	UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningPlayerPawn());
// 	if (OwnerASC)
// 	{
// 		OwnerASC->AbilityCommittedCallbacks.AddUObject(this, &UAbilityGauge::AbilityCommitted);
// 		OwnerASC->AbilitySpecDirtiedCallbacks.AddUObject(this, &UAbilityGauge::AbilitySpecUpdated);
// 		OwnerASC->GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetUpgradePointAttribute()).AddUObject(this, &UAbilityGauge::UpgradePointUpdated);
// 		
// 		bool bFound = false;
// 	
// 		if (bFound)
// 		{
// 			FOnAttributeChangeData ChangeData;
// 			ChangeData.NewValue = UpgradePoint;
// 			UpgradePointUpdated(ChangeData);
// 		}
// 	}
//
//
// 	OwnerAbilitySystemComponent = OwnerASC;
// 	WholeNumberFormattionOptions.MaximumFractionalDigits = 0;
// 	TwoDigitNumberFormattingOptions.MaximumFractionalDigits = 2;
// }
//
// void UReviveWidget::StartCooldown(float CooldownTimeRemaining, float CooldownDuration)
// {
// 	CooldownDurationText->SetText(FText::AsNumber(CooldownDuration));
// 	CachedCooldownDuration = CooldownDuration;
// 	CachedCooldownTimeRemaining = CooldownTimeRemaining;
//
// 	CooldownCounterText->SetVisibility(ESlateVisibility::Visible);
//
// 	GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, this, TimeLimit, CachedCooldownTimeRemaining);
// 	GetWorld()->GetTimerManager().SetTimer(CooldownTimerUpdateHandle, this, &UAbilityGauge::UpdateCooldown, CooldownUpdateInterval, true, 0.f);
//
// }
//
// void UReviveWidget::CooldownFinished()
// {
// }
//
// void UReviveWidget::UpdateCooldown()
// {
// }
//
// const FGameplayAbilitySpec* UReviveWidget::GetAbilitySpec()
// {
// }
