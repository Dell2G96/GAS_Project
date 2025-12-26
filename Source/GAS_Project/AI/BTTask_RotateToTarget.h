// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RotateToTarget.generated.h"

/**
 * 
 */
struct FRotateToTargetTaskMemory
{
	TWeakObjectPtr<APawn> OwningPawn;
	TWeakObjectPtr<AActor> TargetActor;

	bool IsValid() const
	{
		return OwningPawn.IsValid() && TargetActor.IsValid();
	}

	void Reset()
	{
		OwningPawn.Reset();
		TargetActor.Reset();
	}
};


UCLASS()
class GAS_PROJECT_API UBTTask_RotateToTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_RotateToTarget();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual FString GetStaticDescription() const override;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	bool HasReachedAnglePrecision(APawn* QueryPawn, AActor* TargetActor) const;

	UPROPERTY(EditAnywhere, Category="GAS|FaceTarget")
	float AnglePrecision = 5.f;

	UPROPERTY(EditAnywhere, Category="GAS|FaceTarget")
	float RotationInterSpeed = 5.f;

	UPROPERTY(EditAnywhere, Category="GAS|FaceTarget")
	FBlackboardKeySelector InTargetToFaceKey;
};
