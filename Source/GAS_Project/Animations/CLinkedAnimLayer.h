// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CAniminstance.h"
#include "CLinkedAnimLayer.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UCLinkedAnimLayer : public UCAniminstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, meta=(BlueprintThreadSafe))
	class UCPlayerAnim* GetPlayerAnimInstance() const;
};
