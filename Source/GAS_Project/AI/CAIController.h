// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "CAIController.generated.h"

UCLASS()
class GAS_PROJECT_API ACAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACAIController(const FObjectInitializer& ObjectInitializer);

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UAIPerceptionComponent* PerceptionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UAISenseConfig_Sight* AISenseConfig_Sight;

	UFUNCTION()
	virtual void OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
	UPROPERTY(EditDefaultsOnly, Category="GAS|AI")
	bool bEnableDetourCrowdAvoidance = true;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|AI", meta = (EditCondition = "bEnableDetourCrowdAvoidance",UIMin = "1",UIMax = "4"))
	int32 DetourCrowdAvoidanceQuality = 4;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|AI", meta = (EditCondition = "bEnableDetourCrowdAvoidance"))
	float CollisionQueryRange = 600.f;
	
	
	

};
