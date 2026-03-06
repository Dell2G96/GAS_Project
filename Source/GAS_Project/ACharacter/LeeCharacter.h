// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ModularCharacter.h"
#include "GameFramework/Character.h"
#include "LeeCharacter.generated.h"

UCLASS()
class GAS_PROJECT_API ALeeCharacter : public AModularCharacter, public  IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALeeCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void OnAbilitySystemInitialized();
	void OnAbilitySystemUnInitialized();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) final;


	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;


	

public:
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character")
	// TSubclassOf<class ULeeInputComponent> InputComponentClass;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character")
	TObjectPtr<class ULeePawnExtensionComponent> PawnExtComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character")
	TObjectPtr<class ULeeCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character")
	TObjectPtr<class ULeeHealthComponent> HealthComponent;
};
