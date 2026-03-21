// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LeeCharacterMovementComponent.generated.h"

USTRUCT(BlueprintType)
struct FLeeCharacterGroundInfo
{
	GENERATED_BODY()
	FLeeCharacterGroundInfo()
		: LastUpdateFrame(0)
		, GroundDistance(0.0f)
	{}

	uint64 LastUpdateFrame;

	UPROPERTY(BlueprintReadOnly)
	FHitResult GroundHitResult;

	UPROPERTY(BlueprintReadOnly)
	float GroundDistance;
	
};


UCLASS(Config = Game)
class GAS_PROJECT_API ULeeCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	ULeeCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	 virtual void SimulateMovement(float DeltaTime) override;

	 virtual bool CanAttemptJump() const override;

	// Returns the current ground info.  Calling this will update the ground info if it's out of date.
	UFUNCTION(BlueprintCallable, Category = "Lee|CharacterMovement")
	 const FLeeCharacterGroundInfo& GetGroundInfo();

	 void SetReplicatedAcceleration(const FVector& InAcceleration);

	//~UMovementComponent interface
	 virtual FRotator GetDeltaRotation(float DeltaTime) const override;
	 virtual float GetMaxSpeed() const override;
	//~End of UMovementComponent interface

protected:

	virtual void InitializeComponent() override;

protected:

	// Cached ground info for the character.  Do not access this directly!  It's only updated when accessed via GetGroundInfo().
	FLeeCharacterGroundInfo CachedGroundInfo;

	UPROPERTY(Transient)
	bool bHasReplicatedAcceleration = false;
};
