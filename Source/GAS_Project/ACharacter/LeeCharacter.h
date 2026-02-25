// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LeeCharacter.generated.h"

UCLASS()
class GAS_PROJECT_API ALeeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALeeCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character")
	TObjectPtr<class ULeePawnExtensionComponent> PawnExtComponent;

	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lee|Character")
	// TObjectPtr<UHakCameraComponent> CameraComponent;
};
