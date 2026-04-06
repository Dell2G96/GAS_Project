// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/AAbilitySystem/Abilities/LeeGameplayAbility.h"
#include "GAS_Project/AInteraction/InteractionOption.h"
#include "LeeGameplayAbility_Interact.generated.h"

class UIndicatorDescriptor;
class UObject;
class UUserWidget;
struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayEventData;

UCLASS(Abstract)
class GAS_PROJECT_API ULeeGameplayAbility_Interact : public ULeeGameplayAbility
{
	GENERATED_BODY()
	
public:
	ULeeGameplayAbility_Interact(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	UFUNCTION(BlueprintCallable)
	void UpdateInteractions(const TArray<FInteractionOption>& InteractiveOptions );
	
	UFUNCTION(BlueprintCallable)
	void TriggerInteraction();
	
protected:
	UPROPERTY(BlueprintReadWrite)
	TArray<FInteractionOption> CurrentOptions;
	
	UPROPERTY()
	TArray<TObjectPtr<UIndicatorDescriptor>> Indicators;
	
protected:
	UPROPERTY(EditDefaultsOnly)
	float InteractionScanRate = 0.1f;
	
	UPROPERTY(EditDefaultsOnly)
	float InteractionScanRange = 500;
	
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UUserWidget> DefaultInteractionWidgetClass;
	
};
