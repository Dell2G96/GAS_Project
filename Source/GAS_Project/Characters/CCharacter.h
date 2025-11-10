// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"

#include "CCharacter.generated.h"

UCLASS()
class GAS_PROJECT_API ACCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const;
protected:
	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="A_|AbilitySystem")
	class UCAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="A_|AbilitySystem")
	class UCAttributeSet* AttributeSet;

	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="A_|AbilitySystem")
	// TSoftObjectPtr<>

public:
	FORCEINLINE UCAbilitySystemComponent* GetWarriorAbilitySystemComponent() const {return AbilitySystemComponent;}

	FORCEINLINE UCAttributeSet* GetWarriorAttributeSet() const {return AttributeSet;}
};
