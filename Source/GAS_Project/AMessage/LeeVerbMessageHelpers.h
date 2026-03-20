// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LeeVerbMessageHelpers.generated.h"


UCLASS()
class GAS_PROJECT_API ULeeVerbMessageHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Lee")
	static class APlayerState* GetPlayerStateFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category="Lee")
	static APlayerController* GetPlayerControllerFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category="Lee")
	static  struct FGameplayCueParameters VerbMessageToCueParameters(const FLeeVerbMessage& Message);

	UFUNCTION(BlueprintCallable, Category="Lee")
	static FLeeVerbMessage CueParametersToVerbMessage(const FGameplayCueParameters& Params);
};
