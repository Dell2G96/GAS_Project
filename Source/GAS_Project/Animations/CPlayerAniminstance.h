// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CAniminstance.h"
#include "CPlayerAniminstance.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UCPlayerAniminstance : public UCAniminstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE bool GetIsJumping() const { return bIsJumping; }

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE bool GetIsGround() const { return !bIsJumping; }

protected:
	UPROPERTY(BlueprintReadOnly,Category="AnimData")
	class ACCharacter* OwningCharacter;

	UPROPERTY()
	class UCharacterMovementComponent* OwningMovementComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "LocomotionData")
	float GroundSpeed;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "LocomotionData")
	bool bHasAcceleration;

	bool bIsJumping;
	bool bIsGrounded;

	
	
};
