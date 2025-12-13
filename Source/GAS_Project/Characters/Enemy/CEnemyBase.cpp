// Fill out your copyright notice in the Description page of Project Settings.


#include "CEnemyBase.h"

#include "AIController.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"



ACEnemyBase::ACEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;
	CAbilitySystemComponent = CreateDefaultSubobject<UCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	CAbilitySystemComponent->SetIsReplicated(true);
	CAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	CAttributeSet = CreateDefaultSubobject<UCAttributeSet>(TEXT("AttributeSet"));
}

void ACEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(this, this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());

	BindGASChangeDelegate();

	if (!HasAuthority()) return;
	CAbilitySystemComponent->ServerSideInit();
   	
	CAttributeSet = Cast<UCAttributeSet>(GetAttributeSet());
	if (!IsValid(CAttributeSet)) return;

	//ConfigureOverHeadStatusWidget();

	// GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(CAttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	// GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(CAttributeSet->GetStaminaAttribute()).AddUObject(this, &ThisClass::OnStaminaChanged);
	
}

UAbilitySystemComponent* ACEnemyBase::GetAbilitySystemComponent() const
{
	return CAbilitySystemComponent;
}

UAttributeSet* ACEnemyBase::GetAttributeSet() const
{
	return CAttributeSet;
}

void ACEnemyBase::HandleDeath()
{
	Super::HandleDeath();
	
	UE_LOG(LogTemp,Warning,TEXT("ACEnemyBase::HandleDeath called"));
	
	AAIController* AIController = GetController<AAIController>();
	if (!IsValid(AIController)) return;
	AIController->StopMovement();
}

void ACEnemyBase::OnDead()
{
	//ToDo	
}

void ACEnemyBase::OnRespawn()
{
	//ToDo
}


