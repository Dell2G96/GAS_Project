// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "Engine/TimerHandle.h"
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

	/**
	 * 어빌리티가 임시 카메라 모드를 설정한다 (예: 처형/암살 시네마틱 카메라).
	 * 어빌리티 종료 시(EndAbility 모든 경로) 반드시 ClearAbilityCameraMode()를 호출할 것.
	 * @param FocusTarget  카메라가 바라볼 상대 (피니셔의 피해자). 없으면 nullptr
	 * @param MaxDuration  안전망 — 이 시간(초)이 지나면 Clear가 누락돼도 자동 해제. 0 이하면 비활성
	 */
	void SetAbilityCameraMode(TSubclassOf<class ULeeCameraMode> CameraMode, const FGameplayAbilitySpecHandle& OwningSpecHandle, AActor* FocusTarget = nullptr, float MaxDuration = 0.0f);

	/** 설정했던 어빌리티(SpecHandle 일치)의 요청만 해제한다 — 다른 어빌리티의 카메라 모드를 지우는 사고 방지 */
	void ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle);

	/** 어빌리티 카메라 모드가 바라볼 상대 (LeeCameraMode_Finisher가 읽는다) */
	AActor* GetAbilityCameraFocusTarget() const { return AbilityCameraFocusTarget.Get(); }

	void InitializePlayerInput(UInputComponent* PlayerInputComponent);
	void Input_Move(const struct FInputActionValue& InputActionValue);
	void Input_LookMouse(const FInputActionValue& InputActionValue);
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);

	/** 타겟 락온 좌/우 전환 — 상태 없는 명령이라 GAS를 거치지 않고 네이티브 입력으로 직접 처리 */
	void Input_TargetLockSwitchLeft(const FInputActionValue& InputActionValue);
	void Input_TargetLockSwitchRight(const FInputActionValue& InputActionValue);

protected:
	UPROPERTY(EditAnywhere)
	TArray<struct FInputMappingContextAndPriority> DefaultInputMappings;

	UPROPERTY()
	TSubclassOf<class ULeeCameraMode> AbilityCameraMode;

	FGameplayAbilitySpecHandle AbilityCameraModeOwningSpecHandle;

	/** 어빌리티 카메라 모드가 바라볼 상대 (피니셔의 피해자) */
	TWeakObjectPtr<AActor> AbilityCameraFocusTarget;

	/** MaxDuration 안전망 타이머 — 만료 시 자동으로 ClearAbilityCameraMode */
	FTimerHandle AbilityCameraModeTimeoutHandle;

	bool bReadyToBindInputs;

	// 26.03.23 23:45 - Input 초기화 여부 확인용 접근자
public:
	bool GetReadyToBindInputs() const { return bReadyToBindInputs; }

	
	
};



