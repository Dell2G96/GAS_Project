// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeCharacter.h"

#include "LeeHealthComponent.h"
#include "LeePawnExtensionComponent.h"
#include "GAS_Project/AAbilitySystem/LeeAbilitySystemComponent.h"
#include "GAS_Project/ACamera/LeeCameraComponent.h"
#include "GAS_Project/AInput/LeeInputComponent.h"

ALeeCharacter::ALeeCharacter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	PawnExtComponent = CreateDefaultSubobject<ULeePawnExtensionComponent>(TEXT("PawnExtComponent"));
	{
		PawnExtComponent->OnAbilitySystemInitialized_RegistedAndCall(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
		
		PawnExtComponent->OnAbilitySystemUnInitialized_Registed(FSimpleMulticastDelegate::FDelegate::CreateUObject(this,&ThisClass::OnAbilitySystemUnInitialized));
	}
	
	// InputComponentClass = ULeeInputComponent::StaticClass();
	

	// CameraComponent 생성
	{
		CameraComponent = CreateDefaultSubobject<ULeeCameraComponent>(TEXT("CameraComponent"));
		CameraComponent->SetRelativeLocation(FVector(-300.0f, 0.0f, 75.0f));
	}

	// HealthComponent 생성
	{
		HealthComponent = CreateDefaultSubobject<ULeeHealthComponent>(TEXT("HealthComponent"));
	}
}

void ALeeCharacter::OnAbilitySystemInitialized()
{
	ULeeAbilitySystemComponent* LeeASC = Cast<ULeeAbilitySystemComponent>(GetAbilitySystemComponent());
	check(LeeASC);

	HealthComponent->InitializeWithAbilitySystem(LeeASC);
}

void ALeeCharacter::OnAbilitySystemUnInitialized()
{
	HealthComponent->UninitializeAbilitySystem();
}

void ALeeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Pawn이 Possess로서, Controller와 PlayerState 정보 접근이 가능한 상태가 되었음:
	// - SetupPlayerInputComponent 확인
	PawnExtComponent->SetUpPlayerInputComponent();

}


UAbilitySystemComponent* ALeeCharacter::GetAbilitySystemComponent() const
{
	return PawnExtComponent->GetLeeAbilitySystemComponent();
}

