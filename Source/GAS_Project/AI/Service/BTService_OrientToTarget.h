// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_OrientToTarget.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UBTService_OrientToTarget : public UBTService
{
	GENERATED_BODY()


	UBTService_OrientToTarget();
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category="GAS|Target")
	FBlackboardKeySelector InTargetKey;

	UPROPERTY(EditAnywhere, Category="GAS|Target")
	float RotationInterpSpeed;
};
