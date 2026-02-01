// Fill out your copyright notice in the Description page of Project Settings.


#include "CEnemyBase.h"

#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AI/CAIController.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "GAS_Project/GAS/CAbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"


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

	ExecutionTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("ExecutionTrigger"));
	ExecutionTrigger->SetupAttachment(RootComponent);
	ExecutionTrigger->SetSphereRadius(100.f);
    
	// 처음에는 비활성화 (조건이 충족될 때만 켬)
	ExecutionTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ExecutionTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	ExecutionTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	CAbilitySystemComponent = CreateDefaultSubobject<UCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	CAbilitySystemComponent->SetIsReplicated(true);
	CAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	CAttributeSet = CreateDefaultSubobject<UCAttributeSet>(TEXT("AttributeSet"));

	TeamID = 2;

	// GetCharacterMovement()->bOrientRotationToMovement = false;
	// GetCharacterMovement()->bUseControllerDesiredRotation = true;
	
}

void ACEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(this, this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());

	BindGASChangeDelegate();
	
	CAbilitySystemComponent->RegisterGameplayTagEvent(MyTags::Status::Groggy).AddUObject(this, &ACEnemyBase::HandleGroggyTagChanged);

	if (!HasAuthority()) return;
	CAbilitySystemComponent->ServerSideInit();

	SetupStrafingReplicationBridge();

	
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

void ACEnemyBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACEnemyBase, bIsStrafing);
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


void ACEnemyBase::OnGroggyStateChanged(bool bIsGroggy)
{
	if (bIsGroggy)
	{
		// 그로기 상태가 되면 오버랩 활성화
		ExecutionTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		GetMovementComponent()->StopMovementImmediately();
		OnExecutionOverlapBegin()
		
		// Todo :: 처형 UI 띄우기
	}
	else
	{
		ExecutionTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ACEnemyBase::HandleGroggyTagChanged(const struct FGameplayTag Tag, int32 NewCount)
{
	if (NewCount != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("=== Groggy State Entered ==="));
		bool bGroggy = true;
		OnGroggyStateChanged(bGroggy);	
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("=== Groggy Never Enrered ==="));
	}
}

void ACEnemyBase::OnExecutionOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ACPlayerCharacter* Player = Cast<ACPlayerCharacter>(OtherActor))
	{
		//Player->SetTargetableEnemy(this); // 플레이어에게 처형 타겟 등록ㅔ
	}
}

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

void ACEnemyBase::SetupStrafingReplicationBridge()
{
	if (!HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// ABP에서 쓰는 정확한 태그 문자열과 동일
	static const FGameplayTag StrafingTag = MyTags::Status::Strafing;

	// 초기값 세팅(이미 태그가 붙어있는 상태로 시작할 수도 있음)
	bIsStrafing = ASC->HasMatchingGameplayTag(StrafingTag);

	// 태그가 새로 붙거나(Count>0) 제거되면(Count==0) 호출
	ASC->RegisterGameplayTagEvent(StrafingTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ACEnemyBase::HandleStrafingTagChanged);

	ForceNetUpdate();
}

void ACEnemyBase::HandleStrafingTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	const bool bNewStrafing = (NewCount > 0);
	if (bIsStrafing == bNewStrafing)
	{
		return;
	}

	bIsStrafing = bNewStrafing;

	// 바로 클라에 반영되게
	ForceNetUpdate();
}

void ACEnemyBase::OnRep_IsStrafing()
{
	// AnimBP는 Tick에서 DoseOwnerHaveTag를 계속 물어보므로
	// 여기서 별도 처리는 필수는 아님
}






