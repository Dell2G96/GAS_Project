// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "GameplayAbilitySpecHandle.h"

#include "LeeHeroComponent.generated.h"


/**
 * 카메라 , 입력 등 플레이어가 제어하는 시스템의 초기화를 처리하는 컴포넌트
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeHeroComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	ULeeHeroComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Lyra|Hero")
	static ULeeHeroComponent* FindHeroComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<ULeeHeroComponent>() : nullptr); }
	
	static const FName NAME_ActorFeatureName;

	static const FName NAME_BindInputsNow;

	virtual void OnRegister() final;
	virtual void BeginPlay() final;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) final;

	virtual FName GetFeatureName() const { return NAME_ActorFeatureName; }
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) final;
	virtual bool CanChangeInitState(class UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const final;
	virtual void HandleChangeInitState(class UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) final;
	virtual void CheckDefaultInitialization() final;

	TSubclassOf<class ULeeCameraMode> DetermineCameraMode() const;
	void InitializePlayerInput(UInputComponent* PlayerInputComponent);
	void Input_Move(const struct FInputActionValue& InputActionValue);
	void Input_LookMouse(const FInputActionValue& InputActionValue);
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);

protected:
	UPROPERTY(EditAnywhere)
	TArray<struct FInputMappingContextAndPriority> DefaultInputMappings;

	UPROPERTY()
	TSubclassOf<class ULeeCameraMode> AbilityCameraMode;

	FGameplayAbilitySpecHandle AbilityCameraModeOwningSpecHandle;

	bool bReadyToBindInputs;

	// 26.03.23 23:45 - Input 초기화 여부 확인용 접근자
public:
	bool GetReadyToBindInputs() const { return bReadyToBindInputs; }

	
	
};



