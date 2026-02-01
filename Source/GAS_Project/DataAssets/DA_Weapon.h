// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DA_Weapon.generated.h"

USTRUCT(BlueprintType)
struct FExecutionAnimSet
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="GAS|Execution")
	class UAnimMontage* ExecutionMontage;

	UPROPERTY(EditAnywhere, Category="GAS|Execution")
	class UAnimMontage* VictimMontage;
	
};


UCLASS()
class GAS_PROJECT_API UDA_Weapon : public UPrimaryDataAsset
{
	GENERATED_BODY()
};
