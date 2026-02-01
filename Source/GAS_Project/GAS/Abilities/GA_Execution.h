// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CGameplayAbility.h"
#include "GA_Execution.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UGA_Execution : public UCGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Execution();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditAnywhere, Category="GAS|Execution" )
	class UAnimMontage* ExecutionMontage;

	UPROPERTY(EditAnywhere, Category="GAS|Execution" )
	TSubclassOf<class UGameplayEffect> ExecutionEffect;

	UFUNCTION()
	void OnExecutionFinished();

private:
	void UpdateMotionWarping(class AActor* Target);
	
};
