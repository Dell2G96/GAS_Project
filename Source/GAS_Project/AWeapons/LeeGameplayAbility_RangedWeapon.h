// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeRangedWeaponInstance.h"
#include "GAS_Project/AEquipment/LeeGameplayAbility_FromEquipment.h"
#include "LeeGameplayAbility_RangedWeapon.generated.h"

UENUM(BlueprintType)
enum class ELeeAbilityTargetingSource : uint8
{
	CameraTowardFocus,
};

UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_RangedWeapon : public ULeeGameplayAbility_FromEquipment
{
	GENERATED_BODY()
public:
	struct FRangedWeaponFiringInput
	{
		FVector StartTrace;
		FVector EndAim;
		FVector AimDir;
		ULeeRangedWeaponInstance* WeaponData = nullptr;
		bool bCanPlayBulletFX = false;

		FRangedWeaponFiringInput()
			: StartTrace(ForceInitToZero)
			, EndAim(ForceInitToZero)
			, AimDir(ForceInitToZero)
		{
			
		}
	};

	
	ULeeGameplayAbility_RangedWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable)
	void StartRangedWeaponTargeting();

	void PerformLocalTargeting(TArray<FHitResult>& OutHits);

	FTransform GetTargetingTransform(APawn* SourcePawn, ELeeAbilityTargetingSource Source);

	FVector GetWeaponTargetingSourceLocation() const;

	void TraceBulletsInCartridge(const FRangedWeaponFiringInput& InputData, TArray<FHitResult>& OutHits);
	
	FHitResult DoSingleBulletTrace(const FVector& StartTrace, const FVector& Endtrace, float SweepRadius, bool bIsSimulated, TArray<FHitResult>& OutHits) const ;

	FHitResult WeaponTrace(const FVector& StartTrace, const FVector& EndTrace, float SweepRadius, bool bIsSimulated, TArray<FHitResult>& OutHitResults) const;

	ECollisionChannel DetermineTraceChannel(FCollisionQueryParams& TraceParams, bool bIsSimulated) const;
	void AddAdditionalTraceIgnoreActors(FCollisionQueryParams& TraceParams) const;
	
	void OnTargetDataReadyCallBack(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag);

	UFUNCTION(BlueprintImplementableEvent)
	void OnRangeWeaponTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetData);

	ULeeRangedWeaponInstance* GetWeaponInstance();
};


