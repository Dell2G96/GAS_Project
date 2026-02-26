// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "InputMappingContext.h"

#include "LeeMappableConfigPair.generated.h"


USTRUCT(BlueprintType)
struct FInputMappingContextAndPriority
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, Category="Input")
	bool bRegisterWithSettings = true;
};

USTRUCT(BlueprintType)
struct  FLeeMappableConfigPair
{
	GENERATED_BODY()
public:
	FLeeMappableConfigPair() = default;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FInputMappingContextAndPriority> MappingContexts;
	
	const TArray<FInputMappingContextAndPriority>& GetMappingContexts() const { return MappingContexts; }

	
	UPROPERTY(EditAnywhere)
	bool bShouldActivateAutomatically = true;
	
};
