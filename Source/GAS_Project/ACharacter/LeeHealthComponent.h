// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkComponent.h"
#include "GameplayEffectTypes.h"

#include "LeeHealthComponent.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FLeeHealth_AttributeChanged, ULeeHealthComponent* , HealthComponent, float, OldValue, float ,NewValue, AActor*, Instigator);

UCLASS()
class GAS_PROJECT_API ULeeHealthComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()
public:
	ULeeHealthComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category="Lee|Health")
	static ULeeHealthComponent* FindHealthComponent(const AActor* Actor);

	UFUNCTION(BlueprintCallable, Category="Lee|Health")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category="Lee|Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category="Lee|Health")
	float GetHealthNormalized() const;

	void InitializeAbilitySystem(class ULeeAbilitySystemComponent* InASC);
	void UninitializeAbilitySystem();

	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	
	
	UPROPERTY()
	TObjectPtr<class ULeeAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const class ULeeHealthSet> HealthSet;

	UPROPERTY(BlueprintAssignable)
	FLeeHealth_AttributeChanged OnHealthChanged;
	
};
