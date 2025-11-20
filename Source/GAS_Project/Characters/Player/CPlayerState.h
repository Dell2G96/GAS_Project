// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "CPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API ACPlayerState : public APlayerState ,public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ACPlayerState();
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual class UAttributeSet* GetAttributeSet() const { return AttributeSet; }
	
private:
	UPROPERTY(VisibleAnywhere, Category="Crash|Abilities")
	TObjectPtr< UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr< UAttributeSet> AttributeSet;
};
