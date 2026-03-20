// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "ModularAIController.generated.h"

UCLASS(Blueprintable)
class MODULARGAMEPLAYACTORS_API AModularAIController : public AAIController
{
	GENERATED_BODY()
public:
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


};
