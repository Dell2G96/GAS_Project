// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeDamagePopStyleNiagara.h"
#include "LeeNumberPopComponent.h"
#include "LeeNumberPopComponent_NiagaraText.generated.h"


UCLASS(Blueprintable)
class GAS_PROJECT_API ULeeNumberPopComponent_NiagaraText : public ULeeNumberPopComponent
{
	GENERATED_BODY()

public:
	ULeeNumberPopComponent_NiagaraText(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ULeeNumberPopComponent 
	virtual void AddNumberPop(const FLeeNumberPopRequest& NewRequest) override;
	// ULeeNumberPopComponent

protected:
	TArray<int32> DamageNumberArray;

	UPROPERTY(EditDefaultsOnly, Category= " Number Pop|Style")
	TObjectPtr<ULeeDamagePopStyleNiagara> Style;
	
	UPROPERTY(EditDefaultsOnly, Category= " Number Pop|Style")
	TObjectPtr<class UNiagaraComponent> NiagaraComp;
};
