// Fill out your copyright notice in the Description page of Project Settings.


#include "CEnemyBase.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AI/CAIController.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "GAS_Project/Characters/Player/CPlayerController.h"
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
	ExecutionTrigger->SetCollisionEnabled(ECollisionEnabled::Type::QueryOnly);
	ExecutionTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	ExecutionTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ExecutionTrigger->SetGenerateOverlapEvents(true);

	
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
	


	// ✅ WidgetComponent에 Widget Class 할당
	if (ExecutionWidgetComponent && ExecutionUIClass)
	{
		ExecutionWidgetComponent->SetWidgetClass(ExecutionUIClass);
		ExecutionWidgetComponent->SetVisibility(false);
	}
	
	if (HasAuthority())
	{
		CAbilitySystemComponent->ServerSideInit();
		SetupStrafingReplicationBridge();

		CAbilitySystemComponent->RegisterGameplayTagEvent(MyTags::Status::Groggy).AddUObject(this, &ACEnemyBase::HandleGroggyTagChanged);
		
		if (ExecutionTrigger)
		{
			ExecutionTrigger->SetGenerateOverlapEvents(true);
			ExecutionTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnExecutionOverlapBegin);
			ExecutionTrigger->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnExecutionOverlapEnd);
		}	
	}
	else
	{
		if (ExecutionTrigger)
		{
			ExecutionTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	
	
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

	if (HasAuthority())
	{
		EvaluateExecutionForPlayers();
		StopExecutionEvaluationTimer();
	}
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
	if (!HasAuthority()) return ;
	if (bIsGroggy)
	{
		GetMovementComponent()->StopMovementImmediately();
	
	}
	// [ADDED] 그로기 상태 변하면 UI 노출 조건도 변하니 즉시 재평가
	EvaluateExecutionForPlayers();
	
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
		if(GetWorldTimerManager().IsTimerActive(ExecutionEvalTimerHandle))
		{
			StopExecutionEvaluationTimer();
		}
		
	}
}

void ACEnemyBase::OnExecutionOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	
	UE_LOG(LogTemp, Warning, TEXT("Overlap Begin with %s"), *OtherActor->GetName());

	ACPlayerCharacter* Player = Cast<ACPlayerCharacter>(OtherActor);
	if (!Player) return;
	
	PlayersInExecutionRange.Add(Player);

	// [ADDED] 즉시 1회 판정 + 타이머 시작(오버랩 중 회전/타겟 변경 반영)
	SetExecutionUIForPlayer(Player, CanShowExecutionUIFor(Player));
	StartExecutionEvaluationTimer();
		
	
}

void ACEnemyBase::OnExecutionOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	ACPlayerCharacter* Player = Cast<ACPlayerCharacter>(OtherActor);
	if (!Player)
	{
		return;
	}

	PlayersInExecutionRange.Remove(Player); 
	LastExecutionUIState.Remove(Player);

	// 범위에서 나가면 무조건 UI 끄기 + 후보 타겟 해제
	NotifyExecutionUI(Player, false);

	if (PlayersInExecutionRange.Num() == 0)
	{
		StopExecutionEvaluationTimer();
	}
}

FVector ACEnemyBase::GetExecutionUIWorldLocation() const
{
	// [ADDED] 소켓이 있으면 소켓, 없으면 ActorLocation+오프셋
	if (GetMesh() && ExecutionUISocketName != NAME_None && GetMesh()->DoesSocketExist(ExecutionUISocketName))
	{
		return GetMesh()->GetSocketLocation(ExecutionUISocketName) + ExecutionUIOffset;
	}

	// 기본값: 캐릭터 중앙 근처(원하면 Z 오프셋을 ExecutionUIOffset에 넣어 조절)
	return GetActorLocation() + ExecutionUIOffset;
}

bool ACEnemyBase::CanShowExecutionUIFor(class ACPlayerCharacter* Player) const
{
	if (!Player)
	{
		return false;
	}

	// ✅ 1. Groggy 상태 체크
	bool bGroggy = false;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		bGroggy = ASC->HasMatchingGameplayTag(MyTags::Status::Groggy);
	}
    
	if (bGroggy) return true;

	// ✅ 2. BT 타겟팅 체크
	const ACAIController* AIC = Cast<ACAIController>(GetController());
	if (AIC && AIC->GetBlackboardComponent())
	{
		UObject* BBTarget = AIC->GetBlackboardComponent()->GetValueAsObject(BlackboardTargetKeyName);
		if (BBTarget == Player)
		{
			return false; // BT가 타겟팅 중이면 UI 표시 안함
		}
	}

	// ✅ 3. 뒤 60도 각도 체크
	const FVector ToPlayer2D = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const FVector Forward2D = GetActorForwardVector().GetSafeNormal2D();
	const float Dot = FVector::DotProduct(Forward2D, ToPlayer2D);
    
	const float Threshold = FMath::Cos(FMath::DegreesToRadians(180.f - ExecutionBehindAngleDeg));
    
	return (Dot <= Threshold);
	
}

void ACEnemyBase::NotifyExecutionUI(class ACPlayerCharacter* Player, bool bShow)
{
	if (!Player)
	{
		return;
	}

	ACPlayerController* PC = Player->GetController<ACPlayerController>();
	if (!PC)
	{
		return;
	}
	
	//Todo
	PC->Client_SetExecutionCandidate(this, bShow);
}

void ACEnemyBase::StartExecutionEvaluationTimer()
{
	if (!HasAuthority())
	{
		return;
	}

	if (GetWorldTimerManager().IsTimerActive(ExecutionEvalTimerHandle))
	{
		return;
	}

	// [ADDED] Tick 대신 서버 타이머로 충분 (0.1초면 체감 자연스러움 + 비용 낮음)
	GetWorldTimerManager().SetTimer(
		ExecutionEvalTimerHandle,
		this,
		&ThisClass::EvaluateExecutionForPlayers,
		0.1f,
		true
	);
}

void ACEnemyBase::StopExecutionEvaluationTimer()
{
	if (!HasAuthority())
	{
		return;
	}
    
	if (GetWorldTimerManager().IsTimerActive(ExecutionEvalTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(ExecutionEvalTimerHandle);
		UE_LOG(LogTemp, Warning, TEXT("[%s] Execution evaluation timer stopped"), *GetName());
	}
}

void ACEnemyBase::EvaluateExecutionForPlayers()
{
	if (!HasAuthority())
	{
		return;
	}

	bool bIsGroggy = false;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		bIsGroggy = ASC->HasMatchingGameplayTag(MyTags::Status::Groggy);
	}
	
	//유효하지 않은 플레이어 제거를 위해 별도 리스트 사용
	TArray<TWeakObjectPtr<ACPlayerCharacter>> ToRemove;

	for (const TWeakObjectPtr<ACPlayerCharacter>& WeakPlayer : PlayersInExecutionRange)
	{
		ACPlayerCharacter* Player = WeakPlayer.Get();
		if (!IsValid(Player))
		{
			ToRemove.Add(WeakPlayer);
			continue;
		}

		bool bShouldShowUI = false;
		// ✅ 3-1. Groggy 상태라면 무조건 UI 표시
		if (bIsGroggy)
		{
			bShouldShowUI = true;
			UE_LOG(LogTemp, Log, TEXT("[%s] Player %s - UI ON (Enemy is Groggy)"), 
				   *GetName(), *Player->GetName());
		}
		// ✅ 3-2. Groggy 상태가 아니라면 뒤 60도 각도 검사
		else
		{
			// BT 타겟 확인
			bool bIsBTTargeting = false;
			const ACAIController* AIC = Cast<ACAIController>(GetController());
			if (AIC && AIC->GetBlackboardComponent())
			{
				UObject* BBTarget = AIC->GetBlackboardComponent()->GetValueAsObject(BlackboardTargetKeyName);
				bIsBTTargeting = (BBTarget == Player);
			}
			  // BT가 이 플레이어를 타겟팅 중이면 UI 표시 안함
            if (bIsBTTargeting)
            {
                bShouldShowUI = false;
                UE_LOG(LogTemp, Log, TEXT("[%s] Player %s - UI OFF (BT is targeting this player)"), 
                       *GetName(), *Player->GetName());
            }
            else
            {
                // ✅ Enemy 뒤 60도 각도 이내에 있는지 검사
                const FVector ToPlayer2D = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
                const FVector Forward2D = GetActorForwardVector().GetSafeNormal2D();
                const float Dot = FVector::DotProduct(Forward2D, ToPlayer2D);
                
                // "뒤 60도 이내" = 전방 벡터 기준 각도 >= 120도
                // Dot <= cos(120) = -0.5
                const float Threshold = FMath::Cos(FMath::DegreesToRadians(180.f - ExecutionBehindAngleDeg));
                const bool bIsInBehindCone = (Dot <= Threshold);
                
                // 각도 계산 (디버깅용)
                const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));
                
                if (bIsInBehindCone)
                {
                    bShouldShowUI = true;
                    UE_LOG(LogTemp, Log, TEXT("[%s] Player %s - UI ON (Behind %d° cone, Angle=%.1f°, Dot=%.3f)"), 
                           *GetName(), *Player->GetName(), (int32)ExecutionBehindAngleDeg, AngleDeg, Dot);
                }
                else
                {
                    bShouldShowUI = false;
                    UE_LOG(LogTemp, Log, TEXT("[%s] Player %s - UI OFF (Outside cone, Angle=%.1f°, Dot=%.3f)"), 
                           *GetName(), *Player->GetName(), AngleDeg, Dot);
                }
            }
        }
        
        // ✅ 4. UI 상태 업데이트 (변경이 있을 때만 RPC 전송)
        SetExecutionUIForPlayer(Player, bShouldShowUI);
    }
    
    // ✅ 5. 유효하지 않은 플레이어 정리
    for (const TWeakObjectPtr<ACPlayerCharacter>& WeakPlayer : ToRemove)
    {
        PlayersInExecutionRange.Remove(WeakPlayer);
        LastExecutionUIState.Remove(WeakPlayer);
    }
    
    // ✅ 6. 범위 내 플레이어가 없으면 타이머 정지
    if (PlayersInExecutionRange.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] No players in range, stopping evaluation timer"), *GetName());
        StopExecutionEvaluationTimer();
    }
}

void ACEnemyBase::SetExecutionUIForPlayer(ACPlayerCharacter* Player, bool bShow)
{
	if (!Player)
	{
		return;
	}

	TWeakObjectPtr<ACPlayerCharacter> Key(Player);
	bool* Prev = LastExecutionUIState.Find(Key);

	// [ADDED] 상태가 변할 때만 RPC 보내서 스팸 방지
	if (Prev && (*Prev == bShow))
	{
		return;
	}

	LastExecutionUIState.Add(Key, bShow);
	NotifyExecutionUI(Player, bShow);
}
//
// void ACEnemyBase::StartExecutionUITimer()
// {
// 	if (!HasAuthority()) return;
//
// 	if (GetWorldTimerManager().IsTimerActive(ExecutionUITimer)) return;
//
// 	// 0.1초마다 체크
// 	GetWorldTimerManager().SetTimer(ExecutionUITimer, this, &ACEnemyBase::TickExecutionUI, 0.1f, true);
// 	
// }
//
// void ACEnemyBase::StopExecutionUITimer()
// {
// 	if (!HasAuthority()) return;
//
// 	GetWorldTimerManager().ClearTimer(ExecutionUITimer);
// }
//
// void ACEnemyBase::TickExecutionUI()
// {
// 	if (!HasAuthority()) return;
//
// 	// 무효 포인터 정리 + 각 플레이어별 조건 갱신
// 	for (auto It = PlayersInExecutionRange.CreateIterator(); It; ++It)
// 	{
// 		ACPlayerCharacter* Player = It->Get();
// 		if (!IsValid(Player))
// 		{
// 			It.RemoveCurrent();
// 			continue;
// 		}
//
// 		NotifyExecutionUI(Player, CanShowExecutionUIFor(Player));
// 	}
//
// 	if (PlayersInExecutionRange.Num() == 0)
// 	{
// 		StopExecutionUITimer();
// 	}
// }

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






