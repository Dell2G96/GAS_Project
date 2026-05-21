// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeFinishPromptWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "GAS_Project/AUI/IndicatorDescriptor.h"
#include "GAS_Project/MyTags.h"
#include "Materials/MaterialInstanceDynamic.h"

void ULeeFinishPromptWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	CachedProgress = QueryFinishProgress();
	if (RingDynamicMaterial)
	{
		RingDynamicMaterial->SetScalarParameterValue(ProgressMaterialParameterName, CachedProgress);
	}

	OnFinishProgressUpdated(CachedProgress);
}

void ULeeFinishPromptWidget::BindIndicator_Implementation(UIndicatorDescriptor* Indicator)
{
	BoundIndicator = Indicator;
	FinishTarget = Indicator ? Cast<AActor>(Indicator->GetDataObject()) : nullptr;
	RefreshRingMaterial();
	CachedProgress = QueryFinishProgress();
}

void ULeeFinishPromptWidget::UnbindIndicator_Implementation(const UIndicatorDescriptor* Indicator)
{
	if (BoundIndicator == Indicator)
	{
		BoundIndicator = nullptr;
		FinishTarget.Reset();
		RingDynamicMaterial = nullptr;
		CachedProgress = 0.0f;
	}
}

void ULeeFinishPromptWidget::RefreshRingMaterial()
{
	if (!RingImage)
	{
		return;
	}

	RingDynamicMaterial = RingImage->GetDynamicMaterial();
}

float ULeeFinishPromptWidget::QueryFinishProgress() const
{
	AActor* TargetActor = FinishTarget.Get();
	if (!TargetActor)
	{
		return 0.0f;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!ASC)
	{
		return 0.0f;
	}

	const float GroggyProgress = QueryEffectProgress(ASC, MyTags::Souls::Status_Groggy);
	if (GroggyProgress >= 0.0f)
	{
		return GroggyProgress;
	}

	const float LegacyGroggyProgress = QueryEffectProgress(ASC, MyTags::Status::Groggy);
	if (LegacyGroggyProgress >= 0.0f)
	{
		return LegacyGroggyProgress;
	}

	const float UnawareProgress = QueryEffectProgress(ASC, MyTags::Souls::Status_Unaware);
	if (UnawareProgress >= 0.0f)
	{
		return UnawareProgress;
	}

	if (ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Unaware))
	{
		return 1.0f;
	}

	return 0.0f;
}

float ULeeFinishPromptWidget::QueryEffectProgress(UAbilitySystemComponent* ASC, FGameplayTag EffectTag) const
{
	if (!ASC || !EffectTag.IsValid())
	{
		return -1.0f;
	}

	FGameplayEffectQuery Query;
	Query.EffectTagQuery = FGameplayTagQuery::MakeQuery_MatchTag(EffectTag);
	const TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
	if (Handles.IsEmpty())
	{
		return -1.0f;
	}

	const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(Handles[0]);
	if (!ActiveGE)
	{
		return -1.0f;
	}

	const float Duration = ActiveGE->GetDuration();
	if (Duration <= 0.0f)
	{
		return 1.0f;
	}

	const float WorldTime = ASC->GetWorld() ? ASC->GetWorld()->GetTimeSeconds() : 0.0f;
	const float Elapsed = WorldTime - ActiveGE->StartWorldTime;
	const float Remaining = FMath::Max(0.0f, Duration - Elapsed);
	return FMath::Clamp(Remaining / Duration, 0.0f, 1.0f);
}
