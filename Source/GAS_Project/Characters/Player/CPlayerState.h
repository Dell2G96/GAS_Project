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
class GAS_PROJECT_API ACPlayerState : public APlayerState
{
	GENERATED_BODY()
    
public:
	ACPlayerState();
    
	UFUNCTION(BlueprintCallable,Category="GAS|Abilitys")
	class UCAbilitySystemComponent* GetAbilitySystemComponent() const {return AbilitySystemComponent;}
    
	UFUNCTION(BlueprintPure, Category = "GAS|Attributes")
	class UCAttributeSet* GetAttributeSet() const {return AttributeSet;}
    
private:
	UPROPERTY(VisibleAnywhere, Category = "GAS|Abilities")
	TObjectPtr<UCAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "GAS|Attributes")
	TObjectPtr<class UCAttributeSet> AttributeSet;
};
