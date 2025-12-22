// // Fill out your copyright notice in the Description page of Project Settings.
//
// #pragma once
//
// #include "CoreMinimal.h"
// #include "CAniminstance.h"
// #include "CCharacterAniminstance.generated.h"
//
// /**
//  * 
//  */
// UCLASS()
// class GAS_PROJECT_API UCCharacterAniminstance : public UCAniminstance
// {
// 	GENERATED_BODY()
// public:
// 	virtual void NativeInitializeAnimation() override;
// 	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
//
//
// protected:
// 	UPROPERTY(BlueprintReadOnly,Category="AnimData")
// 	class ACCharacter* OwningCharacter;
//
// 	UPROPERTY()
// 	class UCharacterMovementComponent* OwningMovementComp;
//
// 	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "LocomotionData")
// 	float GroundSpeed;
//
// 	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "LocomotionData")
// 	bool bHasAcceleration;
//
// 	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "LocomotionData")
// 	float LocomotionDirection;
// 	
// 	
// };
