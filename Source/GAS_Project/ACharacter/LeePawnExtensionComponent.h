// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "GAS_Project/ACharacter/LeePawnData.h"
#include "LeePawnExtensionComponent.generated.h"

/**
 * 초기화 전반을 조정 하는 컴포넌트
 */
UCLASS()
class GAS_PROJECT_API ULeePawnExtensionComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:
	ULeePawnExtensionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	static const FName NAME_ActorFeatureName;

	static ULeePawnExtensionComponent* FindPawnExtensionComponent(const class AActor* Actor) { return (Actor ? Actor->FindComponentByClass<ULeePawnExtensionComponent>() : nullptr); }

	template<class T>
	const T* GetPawnData() const
	{
		return Cast<T>(PawnData);
	}

	void SetPawnData(const ULeePawnData* InPawnData);
	void SetUpPlayerInputComponent();
	
	class ULeeAbilitySystemComponent* GetLeeAbilitySystemComponent() const
	{
		return AbilitySystemComponent;
	}
	void InitializeAbilitySystem(class ULeeAbilitySystemComponent* InASC, AActor* InOwnerActor);
	void UnInitializeAbilitySystem();

	void OnAbilitySystemInitialized_RegistedAndCall(FSimpleMulticastDelegate::FDelegate Delegate);
	void OnAbilitySystemUnInitialized_Registed(FSimpleMulticastDelegate::FDelegate Delegate);

	// UPawnComponent interfaces
	virtual void OnRegister() final;
	virtual void BeginPlay() final;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) final;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;


	// IGameFrameworkInitStateInterface
	virtual FName GetFeatureName() const final { return NAME_ActorFeatureName; }
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) final;
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const final;
	virtual void CheckDefaultInitialization() final;	
	
	UPROPERTY(EditInstanceOnly, ReplicatedUsing=OnRep_PawnData, Category="LEE|Pawn")
	TObjectPtr<const class ULeePawnData> PawnData;

	UFUNCTION()
	void OnRep_PawnData();

	UPROPERTY(EditInstanceOnly, Category="LEE|AbilitySystem")
	TObjectPtr<class ULeeAbilitySystemComponent> AbilitySystemComponent;

	FSimpleMulticastDelegate OnAbilitySystemInitialized;
	FSimpleMulticastDelegate OnAbilitySystemUnInitialized;
};


