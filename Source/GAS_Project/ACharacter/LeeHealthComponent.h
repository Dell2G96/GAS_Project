// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkComponent.h"
#include "GameplayEffectTypes.h"

#include "LeeHealthComponent.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLeeHealth_DeathEvent, AActor*, OwningActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FLeeHealth_AttributeChanged, ULeeHealthComponent* , HealthComponent, float, OldValue, float ,NewValue, AActor*, Instigator);

UENUM(BlueprintType)
enum class ELeeDeathState : uint8
{
	NotDead = 0,
	DeathStarted,
	DeathFinished
	
};




UCLASS(Blueprintable, Blueprintable,Meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeHealthComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()
public:
	ULeeHealthComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category="Lee|Health")
	static ULeeHealthComponent* FindHealthComponent(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<ULeeHealthComponent>() : nullptr);
	}

	UFUNCTION(BlueprintCallable, Category="Lee|Health")
	void InitializeWithAbilitySystem(class ULeeAbilitySystemComponent* InASC);

	UFUNCTION(BlueprintCallable, Category = "Lee|Health")
	void UninitializeFromAbilitySystem();


	UFUNCTION(BlueprintCallable, Category="Lee|Health")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category="Lee|Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category="Lee|Health")
	float GetHealthNormalized() const;
	
	// Returns the current health value.
	UFUNCTION(BlueprintCallable, Category = "Lee|Health")
	float GetStamina() const;

	// Returns the current maximum health value.
	UFUNCTION(BlueprintCallable, Category = "Lee|Health")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintCallable, Category = "Lee|Health")
	float GetStaminaNormalized() const;

	UFUNCTION(BlueprintCallable, Category = "Lee|Health")
	ELeeDeathState GetDeathState() const { return DeathState; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Lee|Health", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool IsDeadOrDying() const { return (DeathState > ELeeDeathState::NotDead); }

	virtual void StartDeath();

	// Ends the death sequence for the owner.
	virtual void FinishDeath();
	
	
	void UninitializeAbilitySystem();

	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);


	//26.03.19 추가
	virtual void DamageSelfDestruct(bool bFellOutOfWorld = false);

	UPROPERTY(BlueprintAssignable)
	FLeeHealth_AttributeChanged OnHealthChanged;

	
public:



	// Delegate fired when the max health value has changed. This is called on the client but the instigator may not be valid
	UPROPERTY(BlueprintAssignable)
	FLeeHealth_AttributeChanged OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FLeeHealth_AttributeChanged OnStaminaChanged;

	// Delegate fired when the max health value has changed. This is called on the client but the instigator may not be valid
	UPROPERTY(BlueprintAssignable)
	FLeeHealth_AttributeChanged OnMaxStaminaChanged;

	// Delegate fired when the death sequence has started.
	UPROPERTY(BlueprintAssignable)
	FLeeHealth_DeathEvent OnDeathStarted;

	// Delegate fired when the death sequence has finished.
	UPROPERTY(BlueprintAssignable)
	FLeeHealth_DeathEvent OnDeathFinished;

	// 26.03.21 추가
protected:
	virtual void OnUnregister() override;
	
	void ClearGameplayTags();

	virtual void HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleStaminaChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleMaxStaminaChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	
protected:
	
	UPROPERTY()
	TObjectPtr<class ULeeAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const class ULeeSoulsStatSet> HealthSet;

	UPROPERTY()
	ELeeDeathState DeathState;


	
};
