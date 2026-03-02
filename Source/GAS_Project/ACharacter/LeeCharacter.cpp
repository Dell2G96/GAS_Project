// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeCharacter.h"

#include "LeePawnExtensionComponent.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/ACamera/LeeCameraComponent.h"
#include "GAS_Project/AInput/LeeInputComponent.h"


ALeeCharacter::ALeeCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	

	PawnExtComponent = CreateDefaultSubobject<ULeePawnExtensionComponent>(TEXT("PawnExtComponent"));
	
	// InputComponentClass = ULeeInputComponent::StaticClass();

	// CameraComponent 생성
	{
		CameraComponent = CreateDefaultSubobject<ULeeCameraComponent>(TEXT("CameraComponent"));
		CameraComponent->SetRelativeLocation(FVector(-300.0f, 0.0f, 75.0f));
	}

}

UAbilitySystemComponent* ALeeCharacter::GetAbilitySystemComponent() const
{
	return PawnExtComponent->GetLeeAbilitySystemComponent();
}

void ALeeCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ALeeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ALeeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PawnExtComponent->SetUpPlayerInputComponent();

	
}

