// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeFinishTargetComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AUI/IndicatorDescriptor.h"
#include "GAS_Project/AUI/IndicatorLibrary.h"
#include "GAS_Project/System/LeeIndicatorManagerComponent.h"

ULeeFinishTargetComponent::ULeeFinishTargetComponent()
{
	// 박스 진입/상태변화 이벤트로 후보 갱신을 받지만, Tick으로 가벼운 GC 정리만 추가
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f;

	ExecutionTriggerTag = MyTags::Abilities::Execution;
	AssassinationTriggerTag = MyTags::Souls::Ability_Assassination;
}

void ULeeFinishTargetComponent::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 레벨에 이미 있는 Enemy들의 컴포넌트를 자동 구독
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor == GetOwner())
		{
			continue;
		}

		if (ULeeFinishInteractionComponent* EnemyComp =
			Actor->FindComponentByClass<ULeeFinishInteractionComponent>())
		{
			RegisterEnemyComponent(EnemyComp);
		}
	}
}

void ULeeFinishTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// SubscribedEnemyComponents 전체 순회 — Candidates에 없는 컴포넌트도 빠짐없이 해제
	for (const TWeakObjectPtr<ULeeFinishInteractionComponent>& Weak : SubscribedEnemyComponents)
	{
		if (ULeeFinishInteractionComponent* EnemyComp = Weak.Get())
		{
			EnemyComp->OnFinishCandidateEntered.RemoveDynamic(this, &ULeeFinishTargetComponent::OnEnemyCandidateEntered);
			EnemyComp->OnFinishCandidateLeft.RemoveDynamic(this, &ULeeFinishTargetComponent::OnEnemyCandidateLeft);
		}
	}
	SubscribedEnemyComponents.Reset();
	Candidates.Reset();

	DetachIndicator();
	CurrentTarget.Reset();
	CurrentType = ELeeFinishType::None;

	Super::EndPlay(EndPlayReason);
}

void ULeeFinishTargetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// GC된 Enemy/Comp가 섞여있으면 주기적으로 정리
	const int32 Before = Candidates.Num();
	Candidates.RemoveAll([](const FFinishCandidate& C)
	{
		return !C.Enemy.IsValid() || !C.SourceComp.IsValid();
	});

	if (Before != Candidates.Num())
	{
		RecomputeTarget();
	}
}

void ULeeFinishTargetComponent::RegisterEnemyComponent(ULeeFinishInteractionComponent* EnemyComp)
{
	if (!EnemyComp)
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[FinishTargetRegisterEnemyComponent: %s"),*GetNameSafe(EnemyComp->GetOwner()));  // 여기 추가

	if (!EnemyComp->OnFinishCandidateEntered.IsAlreadyBound(this, &ULeeFinishTargetComponent::OnEnemyCandidateEntered))
	{
		EnemyComp->OnFinishCandidateEntered.AddDynamic(this, &ULeeFinishTargetComponent::OnEnemyCandidateEntered);
	}
	if (!EnemyComp->OnFinishCandidateLeft.IsAlreadyBound(this, &ULeeFinishTargetComponent::OnEnemyCandidateLeft))
	{
		EnemyComp->OnFinishCandidateLeft.AddDynamic(this, &ULeeFinishTargetComponent::OnEnemyCandidateLeft);
	}

	SubscribedEnemyComponents.Add(EnemyComp);
}

void ULeeFinishTargetComponent::UnregisterEnemyComponent(ULeeFinishInteractionComponent* EnemyComp)
{
	if (!EnemyComp)
	{
		return;
	}
	EnemyComp->OnFinishCandidateEntered.RemoveDynamic(this, &ULeeFinishTargetComponent::OnEnemyCandidateEntered);
	EnemyComp->OnFinishCandidateLeft.RemoveDynamic(this, &ULeeFinishTargetComponent::OnEnemyCandidateLeft);

	SubscribedEnemyComponents.Remove(EnemyComp);
	Candidates.RemoveAll([EnemyComp](const FFinishCandidate& C) { return C.SourceComp.Get() == EnemyComp; });
	RecomputeTarget();
}

bool ULeeFinishTargetComponent::IsOwningPlayer(AActor* PlayerActor) const
{
	return PlayerActor == GetOwner();
}

void ULeeFinishTargetComponent::OnEnemyCandidateEntered(AActor* EnemyActor, AActor* PlayerActor, ELeeFinishType Type)
{
	if (!IsOwningPlayer(PlayerActor) || !EnemyActor)
	{
		return;
	}

	ULeeFinishInteractionComponent* SourceComp =
		EnemyActor->FindComponentByClass<ULeeFinishInteractionComponent>();

	// 같은 Enemy+Type 중복 방지
	for (FFinishCandidate& Existing : Candidates)
	{
		if (Existing.Enemy.Get() == EnemyActor && Existing.Type == Type)
		{
			return;
		}
	}

	FFinishCandidate Candidate;
	Candidate.SourceComp = SourceComp;
	Candidate.Enemy = EnemyActor;
	Candidate.Type = Type;
	Candidate.EnterTimeSeconds = FPlatformTime::Seconds();
	Candidates.Add(Candidate);

	RecomputeTarget();
}

void ULeeFinishTargetComponent::OnEnemyCandidateLeft(AActor* EnemyActor, AActor* PlayerActor, ELeeFinishType Type)
{
	if (!IsOwningPlayer(PlayerActor) || !EnemyActor)
	{
		return;
	}

	Candidates.RemoveAll([EnemyActor, Type](const FFinishCandidate& C)
	{
		return C.Enemy.Get() == EnemyActor && C.Type == Type;
	});

	RecomputeTarget();
}

void ULeeFinishTargetComponent::RecomputeTarget()
{
	if (Candidates.Num() == 0)
	{
		SetCurrentTarget(nullptr, ELeeFinishType::None);
		return;
	}

	const AActor* Owner = GetOwner();
	const FVector OwnerLoc = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;

	auto ScoreLower = [this, OwnerLoc](const FFinishCandidate& A, const FFinishCandidate& B) -> bool
	{
		switch (PriorityRule)
		{
		case ELeeFinishPriorityRule::AssassinationFirstThenDistance:
		{
			if (A.Type != B.Type)
			{
				// Assassination을 Execution보다 우선
				return A.Type == ELeeFinishType::Assassination;
			}
			const AActor* Ea = A.Enemy.Get();
			const AActor* Eb = B.Enemy.Get();
			const float DistA = Ea ? FVector::DistSquared(OwnerLoc, Ea->GetActorLocation()) : FLT_MAX;
			const float DistB = Eb ? FVector::DistSquared(OwnerLoc, Eb->GetActorLocation()) : FLT_MAX;
			return DistA < DistB;
		}
		case ELeeFinishPriorityRule::DistanceOnly:
		{
			const AActor* Ea = A.Enemy.Get();
			const AActor* Eb = B.Enemy.Get();
			const float DistA = Ea ? FVector::DistSquared(OwnerLoc, Ea->GetActorLocation()) : FLT_MAX;
			const float DistB = Eb ? FVector::DistSquared(OwnerLoc, Eb->GetActorLocation()) : FLT_MAX;
			return DistA < DistB;
		}
		case ELeeFinishPriorityRule::LastEntered:
			return A.EnterTimeSeconds > B.EnterTimeSeconds;
		default:
			return false;
		}
	};

	const FFinishCandidate* Best = &Candidates[0];
	for (int32 i = 1; i < Candidates.Num(); ++i)
	{
		if (ScoreLower(Candidates[i], *Best))
		{
			Best = &Candidates[i];
		}
	}

	SetCurrentTarget(Best->Enemy.Get(), Best->Type);
}

void ULeeFinishTargetComponent::SetCurrentTarget(AActor* NewTarget, ELeeFinishType NewType)
{
	if (CurrentTarget.Get() == NewTarget && CurrentType == NewType)
	{
		return;
	}

	CurrentTarget = NewTarget;
	CurrentType = NewType;

	DetachIndicator();
	if (NewTarget)
	{
		AttachIndicatorTo(NewTarget);
	}

	OnFinishTargetChanged.Broadcast(NewTarget, NewType);
}

void ULeeFinishTargetComponent::AttachIndicatorTo(AActor* Target)
{
	if (!Target || FinishPromptWidgetClass.IsNull())
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PC)
	{
		return;
	}

	ULeeIndicatorManagerComponent* IndicatorMgr = UIndicatorLibrary::GetIndicatorManagerComponent(PC);
	if (!IndicatorMgr)
	{
		return;
	}

	// Enemy의 FinishInteractionComponent에서 소켓명 획득 (기본 spine_03 = 가슴)
	FName SocketName = TEXT("spine_03");
	if (ULeeFinishInteractionComponent* EnemyComp =
		Target->FindComponentByClass<ULeeFinishInteractionComponent>())
	{
		SocketName = EnemyComp->GetIndicatorSocketName();
	}

	USceneComponent* AttachComp = Target->GetRootComponent();
	if (ACharacter* Character = Cast<ACharacter>(Target))
	{
		AttachComp = Character->GetMesh();
	}

	UIndicatorDescriptor* Desc = NewObject<UIndicatorDescriptor>(this);
	Desc->SetIndicatorClass(FinishPromptWidgetClass);
	Desc->SetSceneComponent(AttachComp);
	Desc->SetComponentSocketName(SocketName);
	Desc->SetDataObject(Target);
	Desc->SetWorldPositionOffset(IndicatorWorldOffset);
	Desc->SetAutoRemoveWhenIndicatorComponentIsNull(true);
	Desc->SetProjectionMode(EActorCanvasProjectionMode::ComponentPoint);
	Desc->SetHAlign(HAlign_Center);
	Desc->SetVAlign(VAlign_Center);
	Desc->SetDesiredVisibility(true);

	IndicatorMgr->AddIndicator(Desc);
	ActiveIndicator = Desc;
}

void ULeeFinishTargetComponent::DetachIndicator()
{
	if (!ActiveIndicator)
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (PC)
	{
		if (ULeeIndicatorManagerComponent* IndicatorMgr = UIndicatorLibrary::GetIndicatorManagerComponent(PC))
		{
			IndicatorMgr->RemoveIndicator(ActiveIndicator);
		}
	}

	ActiveIndicator = nullptr;
}

bool ULeeFinishTargetComponent::TryActivateFinish()
{
	
	UE_LOG(LogTemp, Warning,TEXT("[TryActivateFinish] Target=%s / Type=%d / Tag=%s"),*GetNameSafe(CurrentTarget.Get()),  (int32)CurrentType,*AssassinationTriggerTag.ToString());       
	
	AActor* Target = CurrentTarget.Get();
	if (!Target || CurrentType == ELeeFinishType::None)
	{
		UE_LOG(LogTemp, Warning,TEXT("[TryActivateFinish] FAIL: Target 또는Type 없음"));
		return false;
	}

	// 서버 측 ValidateTarget에 태울 최종 판정은 어빌리티가 다시 검증.
	// 여기서는 입력 → GameplayEvent 전달만 수행.
	FGameplayTag TriggerTag = (CurrentType == ELeeFinishType::Assassination)
		? AssassinationTriggerTag : ExecutionTriggerTag;

	if (!TriggerTag.IsValid())
	{
		return false;
	}

	AActor* Owner = GetOwner();
	FGameplayEventData Payload;
	Payload.EventTag = TriggerTag;
	Payload.Instigator = Owner;
	Payload.Target = Target;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, TriggerTag, Payload);
	return true;
}
