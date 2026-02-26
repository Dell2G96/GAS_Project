// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeCameraMode.h"
#include "LeeCameraMode_ThirdPerson.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class GAS_PROJECT_API ULeeCameraMode_ThirdPerson : public ULeeCameraMode
{
	GENERATED_BODY()

public:
	ULeeCameraMode_ThirdPerson(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void UpdateView(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, Category="Third Person")
	TObjectPtr<const class UCurveVector> TargetOffsetCurve;
	
};
