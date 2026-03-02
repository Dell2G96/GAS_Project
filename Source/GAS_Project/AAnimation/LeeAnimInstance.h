// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	UPROPERTY(BlueprintReadOnly, Category="Character State Data")
	float GroundDistance = -1.f;
	
};
