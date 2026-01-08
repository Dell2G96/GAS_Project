// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CGameplayAbility.h"
#include "CTargetLock_Ability.generated.h"

class AActor;
class UUserWidget;
class UInputMappingContext;


UCLASS()
class GAS_PROJECT_API UCTargetLock_Ability : public UCGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

public:
	UFUNCTION(BlueprintCallable, Category="Ability|TargetLock")
	void OnTargetLockTick(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category="Ability|TargetLock")
	void SwitchTarget(const FGameplayTag& InSwitchDirectionTag);

private:
	
	bool TryLockOnTarget();

	void CancelTargetLockAbility();
	void CleanUp();

	
	bool IsOwningLocalController() const;                     
	bool GetViewPoint(FVector& OutLoc, FRotator& OutRot) const; 

	APlayerController* GetOwningPlayerController() const;     
	ACharacter* GetOwningCharacter() const;                   

	
	void GetAvailableActorsToLock();

	AActor* GetInitialTargetByLineTrace() const;             
	AActor* GetTargetClosestToViewCenter(const TArray<AActor*>& InActors) const; 

	AActor* GetTargetToSwitchBySideStable(const TArray<AActor*>& InActors, bool bWantRightSide) const;

	bool IsActorValidForTargetLock(const AActor* InActor) const;
	

	void DrawTargetLockWidget();
	void SetTargetLockWidgetPosition();

	void InitTargetLockMovement();
	void ResetTargetLockMovement();

	void InitTargetMappingContext();
	void ResetTargetLockMappingContext();

private:
	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	float BoxTraceDistance = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	FVector TraceBoxSize = FVector(5000.f, 5000.f, 300.f);

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes;

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	bool bShowPersistentDebug = false;

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	TSubclassOf<UUserWidget> TargetLockWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	float TargetLockRotationInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	float TargetLockMaxWalkSpeed = 150.f;

	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	UInputMappingContext* TargetLockInputContext = nullptr;

	// View-center filtering (fallback + switching)
	UPROPERTY(EditDefaultsOnly, Category="GAS|TargetLock")
	float MinViewCenterDot = 0.15f;

private:

	UPROPERTY()
	TArray<AActor*> AvailableActorsToLock;

	UPROPERTY()
	AActor* CurrentLockedActor = nullptr;

	UPROPERTY()
	UUserWidget* TargetLockWidget = nullptr;

	UPROPERTY()
	FVector2D TargetLockWidgetSize = FVector2D::ZeroVector;

	UPROPERTY()
	float CachedDefaultMaxWalkSpeed = 0.f;

	UPROPERTY()
	bool bMappingContextApplied = false;

	bool bSavedUseControllerRotationYaw  = false;
	bool bSavedOrientRotationToMovement  = true;
	bool bRotationFlagsCached = false;
};
