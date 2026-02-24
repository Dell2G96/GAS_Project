// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LeePlayerState.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ALeePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void PostInitializeComponents() final;

	template<class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }

	void OnExperienceLoaded(const class ULeeExperienceDefinition* CurrentExperience);
	void SetPawnData(const class ULeePawnData* InPawnData);

	UPROPERTY()
	TObjectPtr<const class ULeePawnData> PawnData;
};

