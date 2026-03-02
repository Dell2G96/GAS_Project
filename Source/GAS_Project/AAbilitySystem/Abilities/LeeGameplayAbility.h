// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "LeeGameplayAbility.generated.h"


UENUM(BlueprintType)
enum class ELeeAbilityActivationPolicy : uint8
{
	OnInputTriggered, // Trigger 되었을 경우
	WhileInputActive, // Held 되었을 경우
	OnSpawn			  // avatar가 생성되었을 경우
};

UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	ULeeGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|AbilityActivation")
	ELeeAbilityActivationPolicy ActivationPolicy;
};
