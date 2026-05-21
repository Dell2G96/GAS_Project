// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "LeeGameplayCue_FinishIndicator.generated.h"

class UIndicatorDescriptor;
class UUserWidget;

/**
 * GameplayCue.Souls.FinishIndicator
 * Creates an actor indicator for the local player and lets the widget query GE duration.
 */
UCLASS(Blueprintable)
class GAS_PROJECT_API ALeeGameplayCue_FinishIndicator : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	ALeeGameplayCue_FinishIndicator();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finish")
	TSoftClassPtr<UUserWidget> FinishPromptWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finish")
	FName IndicatorSocketName = TEXT("spine_03");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finish")
	FVector IndicatorWorldOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finish")
	int32 IndicatorPriority = 0;

private:
	UPROPERTY(Transient)
	TObjectPtr<UIndicatorDescriptor> ActiveDescriptor;
};
