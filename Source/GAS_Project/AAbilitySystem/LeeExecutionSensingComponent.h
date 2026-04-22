// Fill out your copyright notice in the Description page of Project Settings.
//
// [DEPRECATED] ULeeFinishInteractionComponent + ULeeFinishTargetComponent로 대체됨.
// BP 참조를 보존하기 위해 껍데기만 유지.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LeeExecutionSensingComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLeeExecutionSensingTargetChanged, AActor*, TargetActor);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DeprecatedNode, DeprecationMessage = "Use ULeeFinishInteractionComponent on Enemy and ULeeFinishTargetComponent on Player instead."))
class GAS_PROJECT_API ULeeExecutionSensingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeeExecutionSensingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lee|Execution", meta = (DeprecatedFunction))
	AActor* GetExecutionTarget() const { return CurrentExecutionTarget.Get(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lee|Execution", meta = (DeprecatedFunction))
	AActor* GetAssassinationTarget() const { return CurrentAssassinationTarget.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "Lee|Execution")
	FOnLeeExecutionSensingTargetChanged OnExecutionTargetChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lee|Execution")
	FOnLeeExecutionSensingTargetChanged OnAssassinationTargetChanged;

private:
	TWeakObjectPtr<AActor> CurrentExecutionTarget;
	TWeakObjectPtr<AActor> CurrentAssassinationTarget;
};
