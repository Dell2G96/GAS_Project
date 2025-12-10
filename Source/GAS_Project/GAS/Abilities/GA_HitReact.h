// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CGameplayAbility.h"
#include "GA_HitReact.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UGA_HitReact : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="GAS|Abilities")
	void CacheHitDirectionVectors(AActor* Instigator);
	
	UPROPERTY(BlueprintReadOnly, Category = "GAS|Abilities")
	FVector AvatarForward;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Abilities")
	FVector ToInstigator;
};
