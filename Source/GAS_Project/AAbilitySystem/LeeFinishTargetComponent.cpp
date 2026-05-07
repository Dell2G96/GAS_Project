// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeFinishTargetComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
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

	// Phase 1 리팩토링: TActorIterator 기반 사전 등록 제거.
	// 이후 Enemy 측 Box Overlap 콜백에서 RegisterEnemyComponent를 직접 호출하도록 전환 예정.
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

void ULeeFinishTargetComponent::AddCandidate(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type)
{
	if (!SourceComp)
	{
		return;
	}

	AActor* Enemy = SourceComp->GetOwner();
	if (!Enemy)
	{
		return;
	}

	for (const FFinishCandidate& Existing : Candidates)
	{
		if (Existing.Enemy.Get() == Enemy && Existing.Type == Type)
		{
			return;
		}
	}

	FFinishCandidate Candidate;
	Candidate.SourceComp = SourceComp;
	Candidate.Enemy = Enemy;
	Candidate.Type = Type;
	Candidate.EnterTimeSeconds = FPlatformTime::Seconds();
	Candidates.Add(Candidate);
	RecomputeTarget();
}

void ULeeFinishTargetComponent::RemoveCandidate(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type)
{
	if (!SourceComp)
	{
		return;
	}

	const int32 Before = Candidates.Num();
	Candidates.RemoveAll([SourceComp, Type](const FFinishCandidate& C)
	{
		return C.SourceComp.Get() == SourceComp && C.Type == Type;
	});

	if (Before != Candidates.Num())
	{
		RecomputeTarget();
	}
}

bool ULeeFinishTargetComponent::HasCandidate(ULeeFinishInteractionComponent* SourceComp, ELeeFinishType Type) const
{
	if (!SourceComp)
	{
		return false;
	}

	for (const FFinishCandidate& C : Candidates)
	{
		if (C.SourceComp.Get() == SourceComp && C.Type == Type)
		{
			return true;
		}
	}
	return false;
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

// ─────────────────────────────────────────────────────────────────────────────
// UI 헬퍼: 위젯 BP에서 라디알 타이머 Progress를 가져오는 함수
// ─────────────────────────────────────────────────────────────────────────────

float ULeeFinishTargetComponent::GetFinishTimerProgress() const
{
	// ① 암살 타입이면 시간제한 없음 → 항상 꽉 찬 링(1.0)
	if (CurrentType == ELeeFinishType::Assassination)
	{
		return 1.0f;
	}

	// ② 타겟이 없으면 0.0
	AActor* Target = CurrentTarget.Get();
	if (!Target)
	{
		return 0.0f;
	}

	// ③ 타겟 ASC 획득
	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!ASC)
	{
		return 0.0f;
	}

	// ④ Groggy GE를 태그 기반으로 쿼리
	//    Souls::Status_Groggy 또는 레거시 Status::Groggy 모두 포함
	FGameplayEffectQuery GroggyQuery;
	GroggyQuery.EffectTagQuery = FGameplayTagQuery::MakeQuery_MatchTag(
		MyTags::Souls::Status_Groggy);

	TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(GroggyQuery);

	// 레거시 태그로도 재시도 (Souls::Status_Groggy로 찾지 못한 경우)
	if (Handles.Num() == 0)
	{
		FGameplayEffectQuery LegacyQuery;
		LegacyQuery.EffectTagQuery = FGameplayTagQuery::MakeQuery_MatchTag(
			MyTags::Status::Groggy);
		Handles = ASC->GetActiveEffects(LegacyQuery);
	}

	if (Handles.Num() == 0)
	{
		// Groggy GE가 없으면 이미 만료됐거나 처형 중 — 0.0 반환
		return 0.0f;
	}

	// ⑤ 잔여시간 / 전체Duration 비율 계산
	//    FActiveGameplayEffectHandle → FActiveGameplayEffect → StartWorldTime & Duration
	const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(Handles[0]);
	if (!ActiveGE)
	{
		return 0.0f;
	}

	const float Duration  = ActiveGE->GetDuration();
	if (Duration <= 0.0f)
	{
		// Duration 이 -1 (Infinite) 이면 풀 링 유지
		return 1.0f;
	}

	const float WorldTime = ASC->GetWorld() ? ASC->GetWorld()->GetTimeSeconds() : 0.0f;
	const float Elapsed   = WorldTime - ActiveGE->StartWorldTime;
	const float Remaining = FMath::Max(0.0f, Duration - Elapsed);

	return FMath::Clamp(Remaining / Duration, 0.0f, 1.0f);
}
