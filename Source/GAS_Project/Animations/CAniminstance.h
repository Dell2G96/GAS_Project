// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "C_BaseAniminstance.h"
#include "Animation/AnimInstance.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "CAniminstance.generated.h"

enum class EWeaponType : uint8;

UCLASS()
class GAS_PROJECT_API UCAniminstance : public UC_BaseAniminstance
{
	GENERATED_BODY()

	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE float GetSpeed() const {return Speed;}

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE bool IsMoving() const {return Speed != 0.0f;}

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE bool IsNotMoving() const {return Speed == 0.0f;}

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE float GetYawSpeed() const {return YawSpeed;}

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE float GetSmoothedYawSpeed() const {return SmoothedYawSpeed;}

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE float GetFwdSpeed() const {return FwdSpeed;}

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE float GetRightSpeed() const {return RightSpeed;}

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE bool GetIsJumping() const {return bIsJumping;}
	
	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE bool GetIsOnGround() const {return !bIsJumping;}

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE float GetLookYawOffset() const {return LookRotOffset.Yaw;}

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE float GetLookPitchOffset() const {return LookRotOffset.Pitch;}

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE float GetMovementDirection() const {return Direction;}

	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE ACharacter* GetOwnerCharacter() const {return OwningCharacter;}
	

	// UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	// bool ShouldDoFullBody() const;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="GAS|Weapon")
	EWeaponType WeaponType = EWeaponType::None;

	UFUNCTION()
	void OnWeaponTypeChanged(EWeaponType InPrevType, EWeaponType InNewType);


protected:
	UPROPERTY(VisibleDefaultsOnly ,Category="AnimData|References")
	class ACharacter* OwningCharacter;

	UPROPERTY(VisibleDefaultsOnly ,Category="AnimData|References")
	class ACharacter* OwningEnemy;

	UPROPERTY(VisibleDefaultsOnly ,Category="AnimData|References")
	class UCharacterMovementComponent* OwnerMovement;
	
	UPROPERTY(EditAnywhere, Category="Animation")
	float YawSpeedSmoothLerpSpeed = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Animation")
	float YawSpeedLerpToZeroSpeed = 30.f;
	
	float Speed;
	float YawSpeed;
	float SmoothedYawSpeed;
	float FwdSpeed;
	float RightSpeed;
	bool bIsJumping;
	float Direction;
	
	FRotator PreBodyRotation;
	FRotator LookRotOffset;

	// Enemy //
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	float LocomotionDirection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	float GroundSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	bool bHasAcceleration;
	
};
