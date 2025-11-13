// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS_Project/Characters/CCharacter.h"
#include "CPlayerCharacter.generated.h"

UCLASS()
class GAS_PROJECT_API ACPlayerCharacter : public ACCharacter
{
	GENERATED_BODY()

public:
	ACPlayerCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
private:
	UPROPERTY(VisibleAnywhere,Category="A|Camera")
	TObjectPtr<class USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere,Category="A|Camera")
	TObjectPtr<class UCameraComponent> Camera;
};
