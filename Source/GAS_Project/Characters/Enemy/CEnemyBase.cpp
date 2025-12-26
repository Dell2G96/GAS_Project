// Fill out your copyright notice in the Description page of Project Settings.


#include "CEnemyBase.h"

#include "AIController.h"
#include "Components/BoxComponent.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"



ACEnemyBase::ACEnemyBase()
{
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	PrimaryActorTick.bCanEverTick = false;
	LeftHandCollision = CreateDefaultSubobject<UBoxComponent>("LeftHandCollision");
	LeftHandCollision->SetupAttachment(GetMesh());
	RightHandCollision = CreateDefaultSubobject<UBoxComponent>("RightHandCollision");
	RightHandCollision->SetupAttachment(GetMesh());
	
	LeftHandCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightHandCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
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

	SetGenericTeamId(2);
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


#if WITH_EDITOR
void ACEnemyBase::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, LeftHandSocket))
	{
		LeftHandCollision->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, LeftHandSocket);
	}

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, RightHandSocket))
	{
		RightHandCollision->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, RightHandSocket);
	}
}
#endif


FGenericTeamId ACEnemyBase::GetGenericTeamId() const
{
	return Super::GetGenericTeamId();
}

void ACEnemyBase::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	Super::SetGenericTeamId(NewTeamID);
}

void ACEnemyBase::OnDead()
{
	//ToDo	
}

void ACEnemyBase::OnRespawn()
{
	//ToDo
}


