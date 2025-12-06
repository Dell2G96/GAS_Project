// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "CPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ACPlayerState : public APlayerState, public  IAbilitySystemInterface
{
	GENERATED_BODY()
    
public:
	ACPlayerState();
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	class UAttributeSet* GetAttributeSet() const {return AttributeSet;}

	// UFUNCTION(BlueprintCallable,Category="GAS|Abilitys")
	// class UCAbilitySystemComponent* GetAbilitySystemComponent2() const {return AbilitySystemComponent;}
 //    
    
private:
	UPROPERTY(VisibleAnywhere, Category = "GAS|Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "GAS|Attributes")
	TObjectPtr<class UAttributeSet> AttributeSet;
};
