// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "InputMappingContext.h"

#include "LeeMappableConfigPair.generated.h"



USTRUCT()
struct  FLeeMappableConfigPair
{
	GENERATED_BODY()
public:
	FLeeMappableConfigPair() = default;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<class UPlayerMappableInputConfig> Config;
	
	UPROPERTY(EditAnywhere)
	bool bShouldActivateAutomatically = true;
	
};
