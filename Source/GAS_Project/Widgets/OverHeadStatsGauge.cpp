// Fill out your copyright notice in the Description page of Project Settings.


#include "OverHeadStatsGauge.h"

#include "AbilitySystemComponent.h"
#include "ValueGauge.h"
#include "GAS_Project/GAS/CAttributeSet.h"


void UOverHeadStatsGauge::ConfigureWithASC(class UAbilitySystemComponent* AbilitySystemComponent)
{
	if (AbilitySystemComponent)
	{
		HealthBar->SetAndBoundToGameplayAttribute(AbilitySystemComponent,UCAttributeSet::GetHealthAttribute(),UCAttributeSet::GetMaxHealthAttribute());
		ManaBar->SetAndBoundToGameplayAttribute(AbilitySystemComponent, UCAttributeSet::GetManaAttribute(), UCAttributeSet::GetMaxManaAttribute());
	}
}
