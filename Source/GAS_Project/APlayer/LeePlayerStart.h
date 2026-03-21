// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/PlayerStart.h"
#include "LeePlayerStart.generated.h"


enum class ELeePlayerStartLocationOccupancy
{
	Empty,
	Partial,
	Full
};


UCLASS(Config=Game)
class GAS_PROJECT_API ALeePlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	ALeePlayerStart(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	const FGameplayTagContainer& GetGameplayTags() {return StartPointTags;}

	ELeePlayerStartLocationOccupancy GetLocationOccupancy(AController* const ControllerPawnToFit) const ;

	UFUNCTION(BlueprintCallable)
	bool IsClaimed() const;

	UFUNCTION(BlueprintCallable)
	bool TryClaim(AController* OccupyingController);

protected:
	void CheckUnclaimed();

	
	UPROPERTY(Transient)
	TObjectPtr<AController> ClaimingController = nullptr;

	UPROPERTY(EditDefaultsOnly, Category= "Player Start Claiming")
	float ExpirationCheckInterval = 1.f;

	UPROPERTY(EditAnywhere)
	FGameplayTagContainer StartPointTags;

	FTimerHandle ExpirationTimerHandle;
	
};
