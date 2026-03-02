// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeWeaponInstance.h"
#include "UObject/Object.h"
#include "LeeRangedWeaponInstance.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeRangedWeaponInstance : public ULeeWeaponInstance
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "WeaponConfig", meta=(ForceUnits = cm))
	float MaxDamageRange = 25000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "WeaponConfig", meta=( ForceUnits = cm))
	float BulletTraceWeaponRadius = 0.f;
};
