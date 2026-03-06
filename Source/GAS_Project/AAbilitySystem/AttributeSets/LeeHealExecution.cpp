// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeHealExecution.h"

#include "LeeCombatSet.h"
#include "LeeHealthSet.h"

struct FHealStatics
{
	FGameplayEffectAttributeCaptureDefinition BaseHealDef;
	FHealStatics()
	{
		BaseHealDef = FGameplayEffectAttributeCaptureDefinition(ULeeCombatSet::GetBaseHealAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
	}
};

static FHealStatics& HealStatics()
{
	static FHealStatics Statics;
	return Statics;
}

ULeeHealExecution::ULeeHealExecution()
{
	RelevantAttributesToCapture.Add(HealStatics().BaseHealDef);
}

void ULeeHealExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	float BaseHeal = 0.f;
	{
		FAggregatorEvaluateParameters EvaluateParams;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(HealStatics().BaseHealDef, EvaluateParams, BaseHeal);
	}

	const float HealingDone = FMath::Max(0.f, BaseHeal);
	if (HealingDone > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(ULeeHealthSet::GetHealingAttribute(), EGameplayModOp::Additive, HealingDone));
	}
}
