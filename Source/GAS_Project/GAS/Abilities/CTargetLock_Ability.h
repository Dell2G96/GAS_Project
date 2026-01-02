// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CGameplayAbility.h"
#include "CTargetLock_Ability.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UCTargetLock_Ability : public UCGameplayAbility
{
	GENERATED_BODY()


	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void OnTargetLockTick(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void SwitchTarget(const FGameplayTag& InSwitchDirectionTag);

private:
	void TryLockOnTarget();
	void GetAvailableActorsToLock();
	AActor* GetNearestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors);
	void GetAvailableActorsAroundTarget(TArray<AActor*>& OutActorsOnLeft,TArray<AActor*>& OutActorsOnRight );
	void DrawTargetLockWidget();
	void SetTargetLockMovement();
	void InitTargetLockMovement();
	void InitTargetMappingContext();


	void CancelTargetLockAbility();
	void CleanUp();
	void ResetTargetLockMovement();
	void ResetTargetLockMappingContext();

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	float BoxTraceDistance = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	FVector TraceBoxSize = FVector(5000.f, 5000.f, 300.f);

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	TArray<TEnumAsByte< EObjectTypeQuery>> BoxTraceChannel;

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	bool bShowPersistentDebugSphere = false;

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	TSubclassOf<class ACCharacter> TargetLockWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	float TargetLockRotationInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	float TargetLockMaxWalkSpeed = 150.f;

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	class UInputMappingContext* TargetLockInputContext;

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	float TargetLockCameraOffsetDistance = 20.f;


	UPROPERTY()
	TArray<AActor*> AvailableActorsToLock;

	UPROPERTY()
	AActor* CurrentLockedActor;


	// 수정 해야 될듯
	UPROPERTY()
	class UUserWidget* TargetLockWidget;

	UPROPERTY()
	FVector2D TargetLockWidgetSize = FVector2D::ZeroVector;

	UPROPERTY()
	float CachedDefaultMaxWalkSpeed = 0.f;

	









	
};
