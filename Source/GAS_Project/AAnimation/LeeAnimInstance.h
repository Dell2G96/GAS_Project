// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Animation/AnimInstance.h"
#include "LeeAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	virtual void NativeInitializeAnimation() override;

	void InitializeWithAbilitySystem(class UAbilitySystemComponent* ASC);

protected:
	// ASC 준비 시점(또는 이미 준비됐으면 즉시)에 호출 → GameplayTag Property Map 초기화
	void OnAbilitySystemInitialized();

public:

	UPROPERTY(BlueprintReadOnly, Category="Character State Data")
	float GroundDistance = -1.f;

	UPROPERTY(EditDefaultsOnly, Category="GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;
		
	
};
