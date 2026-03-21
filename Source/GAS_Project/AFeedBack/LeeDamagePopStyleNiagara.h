// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LeeDamagePopStyleNiagara.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ULeeDamagePopStyleNiagara : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="DamagePop")
	FName NiagaraArrayName;

	UPROPERTY(EditDefaultsOnly, Category="DamagePop")
	TObjectPtr<class UNiagaraSystem> TextNiagara;
};
