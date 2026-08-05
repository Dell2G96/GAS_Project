// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeTargetSelectionComponent.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/AAI/LeeThreatComponent.h"
#include "GAS_Project/AAI/Token/LeeAttackTokenComponent.h"
#include "GAS_Project/ACharacter/LeeEnemySensingComponent.h"
#include "GAS_Project/ACharacter/LeePawnExtensionComponent.h"
#include "GAS_Project/ACharacter/LeeTargetLockTargetComponent.h"
#include "GAS_Project/ATeam/LeeTeamSubsystem.h"
#include "Components/StateTreeComponent.h"

// 타겟 선정 컴포넌트 생성자. 자체 틱 대신 TargetUpdateInterval 주기 타이머를 사용한다
ULeeTargetSelectionComponent::ULeeTargetSelectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 프로젝트의 다른 Lee 컴포넌트들과 동일한 사망 판정 태그를 기본값으로 둔다 (BP에서 교체 가능)
	DeathTag = MyTags::Souls::Status_Death_Dead;

	// [신규] 그로기 판정 태그 기본값 (스태미나 고갈 시 GE_Groggy가 부여하는 태그)
	GroggyTag = MyTags::Souls::Status_Groggy;
}

// 서버에서만 판정 타이머를 시작하고, Possess 변경 알림을 구독해 사망 태그를 따라붙인다 (§6 서버 권위 정책)
void ULeeTargetSelectionComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	// AIController가 Pawn을 빙의/해제할 때마다 알림을 받는다.
	// UnPossess(NewPawn == nullptr)도 여기로 들어오므로 회수 경로 5를 C++만으로 처리할 수 있다.
	if (AController* OwnerController = Cast<AController>(Owner))
	{
		OwnerController->GetOnNewPawnNotifier().AddUObject(this, &ThisClass::HandlePossessedPawnChanged);
		HandlePossessedPawnChanged(OwnerController->GetPawn());
	}

	StartUpdateTimer();
}

// 컴포넌트 종료 시 타이머 정리 + 사망 태그 구독 해제 + 보유 중이던 토큰 반납 (회수 경로 3)
void ULeeTargetSelectionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopUpdateTimer();
	UnbindDeathTag();

	ClearTarget();
	BoundPawn.Reset();

	Super::EndPlay(EndPlayReason);
}

// 판정 타이머 시작 (중복 SetTimer 방지)
void ULeeTargetSelectionComponent::StartUpdateTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(UpdateTimerHandle, this, &ThisClass::UpdateTargetSelection, TargetUpdateInterval, /*bLoop*/true);
	}
}

// 판정 타이머 정지
void ULeeTargetSelectionComponent::StopUpdateTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimerHandle);
	}
}

// Possess/UnPossess 시 호출. 이전 Pawn이 점유하던 토큰을 반납하고 새 Pawn의 사망 태그를 구독한다 (회수 경로 5)
void ULeeTargetSelectionComponent::HandlePossessedPawnChanged(APawn* NewPawn)
{
	APawn* OldPawn = BoundPawn.Get();
	if (OldPawn == NewPawn)
	{
		return;
	}

	// 이전 Pawn 이름으로 발급된 토큰은 여기서 확실히 회수한다
	if (OldPawn)
	{
		ReleaseTokensOn(CurrentTarget.Get(), OldPawn);
	}

	UnbindDeathTag();

	CurrentTarget.Reset();
	bIsDead = false;
	BoundPawn = NewPawn;

	if (NewPawn)
	{
		BindDeathTag(NewPawn);
	}
}

// 새 Pawn의 사망 태그 구독을 예약. ASC가 아직 초기화되지 않았으면 초기화 시점까지 기다린다
void ULeeTargetSelectionComponent::BindDeathTag(APawn* Pawn)
{
	if (!Pawn || !DeathTag.IsValid())
	{
		return;
	}

	// ALeeCharacter 계열은 ASC가 PawnExtensionComponent를 통해 나중에 초기화된다 (ULeeThreatComponent와 동일한 사정)
	if (ULeePawnExtensionComponent* PawnExt = ULeePawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		PawnExt->OnAbilitySystemInitialized_RegistedAndCall(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::BindDeathTagOnBoundPawn));
		return;
	}

	BindDeathTagOnBoundPawn();
}

// 현재 BoundPawn의 ASC에서 DeathTag 변화를 실제로 구독한다 (중복 구독 방지 포함)
void ULeeTargetSelectionComponent::BindDeathTagOnBoundPawn()
{
	APawn* Pawn = BoundPawn.Get();
	if (!Pawn || !DeathTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!ASC)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeTargetSelectionComponent] %s: ASC를 찾지 못해 사망 이벤트를 감지할 수 없습니다."), *GetNameSafe(Pawn));
		return;
	}

	if (BoundASC.Get() == ASC && DeathTagDelegateHandle.IsValid())
	{
		return;
	}

	UnbindDeathTag();

	BoundASC = ASC;
	DeathTagDelegateHandle = ASC->RegisterGameplayTagEvent(DeathTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::HandleDeathTagChanged);

	// [신규] 같은 ASC에서 그로기 태그도 함께 구독한다 (StateTree 이벤트 브리지)
	
	if (GroggyTag.IsValid())
	{
		GroggyTagDelegateHandle = ASC->RegisterGameplayTagEvent(GroggyTag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ThisClass::HandleGroggyTagChanged);
	}
}

// 사망 태그 구독 해제
void ULeeTargetSelectionComponent::UnbindDeathTag()
{
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		if (DeathTagDelegateHandle.IsValid() && DeathTag.IsValid())
		{
			ASC->RegisterGameplayTagEvent(DeathTag, EGameplayTagEventType::NewOrRemoved).Remove(DeathTagDelegateHandle);
		}

		// [신규] 그로기 태그 구독도 함께 해제 
		if (GroggyTagDelegateHandle.IsValid() && GroggyTag.IsValid())
		{
			ASC->RegisterGameplayTagEvent(GroggyTag, EGameplayTagEventType::NewOrRemoved).Remove(GroggyTagDelegateHandle);
		}
	}
	BoundASC.Reset();
	DeathTagDelegateHandle.Reset();
	GroggyTagDelegateHandle.Reset();
}

// [신규] 그로기 태그가 켜지면 토큰을 반납하고 Groggy.Begin, 꺼지면 Groggy.End를 StateTree에 발신한다
void ULeeTargetSelectionComponent::HandleGroggyTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	const bool bNowGroggy = NewCount > 0;
	if (bIsGroggy == bNowGroggy)
	{
		return;
	}
	bIsGroggy = bNowGroggy;

	if (bNowGroggy)
	{
		// 그로기 중에는 공격 슬롯을 물고 있으면 안 된다 (다른 Enemy가 들어올 수 있게 즉시 반납)
		ReleaseTokensOn(CurrentTarget.Get(), BoundPawn.Get());
		SendAIEvent(MyTags::Souls::AIEvent_Groggy_Begin);
		return;
	}

	SendAIEvent(MyTags::Souls::AIEvent_Groggy_End);
}

// 사망 태그가 켜지면 타겟 해제 + 토큰 반납 + Souls.AI.Event.Died 발신, 꺼지면(리스폰) 위협도 초기화 후 재개 (회수 경로 2)
void ULeeTargetSelectionComponent::HandleDeathTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (NewCount > 0)
	{
		if (bIsDead)
		{
			return;
		}
		bIsDead = true;

		StopUpdateTimer();

		// 죽은 Enemy가 점유하던 슬롯을 즉시 비워 다른 Enemy가 공격에 들어갈 수 있게 한다
		ReleaseTokensOn(CurrentTarget.Get(), BoundPawn.Get());
		SetCurrentTarget(nullptr);

		SendAIEvent(MyTags::Souls::AIEvent_Died);
		return;
	}

	// 리스폰 — 위협도를 승계하지 않고 초기화한 뒤 판정을 재개한다 (Q7 결정안)
	if (!bIsDead)
	{
		return;
	}
	bIsDead = false;

	if (ULeeThreatComponent* ThreatComp = GetEnemyThreatComponent())
	{
		ThreatComp->ResetThreat();
	}

	StartUpdateTimer();
}

// Target에 부착된 어택 토큰 컴포넌트에서 Requester의 모든 Claim을 반납 (모든 회수 경로의 공용 진입점)
void ULeeTargetSelectionComponent::ReleaseTokensOn(AActor* Target, APawn* Requester) const
{
	if (!Target || !Requester)
	{
		return;
	}

	if (ULeeAttackTokenComponent* TokenComp = ULeeAttackTokenComponent::FindAttackTokenComponent(Target))
	{
		TokenComp->ReleaseAll(Requester);
	}
}

// [서버] 타겟은 유지한 채 어택 토큰만 강제 반납. 처형/StopLogic 등 StateTree가 정상 종료되지 않는 경로용 (회수 경로 3)
void ULeeTargetSelectionComponent::ForceReleaseAttackTokens()
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	ReleaseTokensOn(CurrentTarget.Get(), BoundPawn.Get());
}

// AIController가 실제로 빙의 중인 Enemy Pawn을 반환
APawn* ULeeTargetSelectionComponent::GetControlledEnemyPawn() const
{
	const AAIController* AIController = Cast<AAIController>(GetOwner());
	return AIController ? AIController->GetPawn() : nullptr;
}

// Enemy Pawn에 부착된 위협도 컴포넌트를 조회
ULeeThreatComponent* ULeeTargetSelectionComponent::GetEnemyThreatComponent() const
{
	APawn* EnemyPawn = GetControlledEnemyPawn();
	return EnemyPawn ? ULeeThreatComponent::FindThreatComponent(EnemyPawn) : nullptr;
}

// 타겟 락온/피니셔와 동일한 기준점 계산 규칙 (ULeeTargetLockComponent::GetFocusLocationFor와 동일 방식)
FVector ULeeTargetSelectionComponent::GetFocusLocationFor(const AActor* Actor) const
{
	if (!Actor)
	{
		return FVector::ZeroVector;
	}

	if (const ULeeTargetLockTargetComponent* FocusComp = ULeeTargetLockTargetComponent::FindTargetLockTargetComponent(Actor))
	{
		return FocusComp->GetFocusLocation();
	}

	return Actor->GetActorLocation();
}

// 유효(생존) 후보인지 확인. 사망 판정은 다른 Lee 컴포넌트들과 동일하게 Lyra::Status_Death_Dead를 사용한다
bool ULeeTargetSelectionComponent::IsValidCandidate(const APawn* Candidate) const
{
	if (!IsValid(Candidate))
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<APawn*>(Candidate));
	if (!ASC)
	{
		return false;
	}

	return !ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Death_Dead);
}

// 다른 팀 소속인지 확인 (ULeeTargetLockComponent::PassesTeamFilter와 동일 방식)
bool ULeeTargetSelectionComponent::PassesTeamFilter(const APawn* Candidate) const
{
	const APawn* EnemyPawn = GetControlledEnemyPawn();
	if (!EnemyPawn || !Candidate)
	{
		return false;
	}

	const ULeeTeamSubsystem* TeamSubsystem = GetWorld() ? GetWorld()->GetSubsystem<ULeeTeamSubsystem>() : nullptr;
	if (!TeamSubsystem)
	{
		return false;
	}

	return TeamSubsystem->CompareTeams(EnemyPawn, Candidate) == ELeeTeamComparison::DifferentTeams;
}

// Enemy Pawn의 SensingComponent가 인지 중인 폰 중 유효 후보(생존 + 다른 팀)만 추려 반환
void ULeeTargetSelectionComponent::GatherCandidates(TArray<APawn*>& OutCandidates) const
{
	OutCandidates.Reset();

	const APawn* EnemyPawn = GetControlledEnemyPawn();
	const ULeeEnemySensingComponent* SensingComp = EnemyPawn ? ULeeEnemySensingComponent::FindEnemySensingComponent(EnemyPawn) : nullptr;
	if (!SensingComp)
	{
		
		return;
	}

	TArray<APawn*> Perceived;
	SensingComp->GetPerceivedPawns(Perceived);

	for (APawn* Candidate : Perceived)
	{
		const bool bValid = IsValidCandidate(Candidate);
		const bool bTeamOk = PassesTeamFilter(Candidate);
		if (bValid && bTeamOk)
		{
			OutCandidates.Add(Candidate);
		}
		else
		{
			
		}
	}

	
}

// AIController 소유 StateTreeComponent에 이벤트 발신 (StateTree가 아직 없거나 비활성 상태면 조용히 무시)
void ULeeTargetSelectionComponent::SendAIEvent(FGameplayTag EventTag) const
{
	if (!EventTag.IsValid())
	{
		return;
	}

	if (UStateTreeComponent* StateTreeComp = GetOwner() ? GetOwner()->FindComponentByClass<UStateTreeComponent>() : nullptr)
	{
		StateTreeComp->SendStateTreeEvent(EventTag);
	}
}

// 실제 타겟 전환 처리. 이전 타겟의 어택 토큰을 반드시 회수한다 (P0-5 회수 경로 4)
void ULeeTargetSelectionComponent::SetCurrentTarget(AActor* NewTarget)
{
	AActor* Old = CurrentTarget.Get();
	if (Old == NewTarget)
	{
		return;
	}

	CurrentTarget = NewTarget;
	LastSwitchTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

	// UnPossess 직후에는 GetControlledEnemyPawn()이 이미 null이므로 캐시해둔 BoundPawn을 우선 사용한다
	APawn* Requester = BoundPawn.IsValid() ? BoundPawn.Get() : GetControlledEnemyPawn();
	ReleaseTokensOn(Old, Requester);

	OnTargetChanged.Broadcast(Old, NewTarget);
}

// [서버] 타겟을 강제로 해제 (토큰 반납 포함, 이벤트는 TargetInvalidated로 발신)
void ULeeTargetSelectionComponent::ClearTarget()
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (!CurrentTarget.IsValid())
	{
		return;
	}

	SetCurrentTarget(nullptr);
	SendAIEvent(MyTags::Souls::AIEvent_TargetInvalidated);
}

// 판정 주기마다 호출되는 타겟 선정 로직 본체 (리뷰 §3-12 수식)
void ULeeTargetSelectionComponent::UpdateTargetSelection()
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || bIsDead)
	{
		
		return;
	}

	APawn* EnemyPawn = GetControlledEnemyPawn();
	if (!EnemyPawn)
	{
	 
		return;
	}

	ULeeThreatComponent* ThreatComp = GetEnemyThreatComponent();
	if (ThreatComp)
	{
		ThreatComp->PruneExpiredThreats();
	}

	TArray<APawn*> Candidates;
	GatherCandidates(Candidates);

	AActor* Current = CurrentTarget.Get();

	// 2. 즉시 재선정 예외 (현재 타겟 사망·무효·후보 제외·팀 변경)
	bool bForceReselect = false;
	if (Current)
	{
		const APawn* CurrentPawn = Cast<APawn>(Current);
		const bool bStillCandidate = Candidates.Contains(CurrentPawn);
		if (!IsValidCandidate(CurrentPawn) || !bStillCandidate)
		{
			bForceReselect = true;
		}
	}

	// 6. 후보 0명 → 타겟 해제 + 토큰 반납 + TargetInvalidated 이벤트
	if (Candidates.Num() == 0)
	{
		if (Current)
		{
			SetCurrentTarget(nullptr);
			SendAIEvent(MyTags::Souls::AIEvent_TargetInvalidated);
		}
		return;
	}

	// 3. 즉시 재선정 사유가 없고 최소 유지 시간 이내면 유지
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (!bForceReselect && Current && (Now - LastSwitchTime) < MinTargetHoldTime)
	{
		return;
	}

	// 4. 1순위 — 위협도
	APawn* BestByThreat = nullptr;
	float BestThreat = -1.f;
	if (ThreatComp)
	{
		for (APawn* Candidate : Candidates)
		{
			const float CandidateThreat = ThreatComp->GetThreat(Candidate);
			if (CandidateThreat > BestThreat)
			{
				BestThreat = CandidateThreat;
				BestByThreat = Candidate;
			}
		}
	}

	const float MinThreshold = ThreatComp ? ThreatComp->MinThreatThreshold : 0.f;

	AActor* NewTarget = Current;

	if (BestByThreat && BestThreat >= MinThreshold)
	{
		if (!Current || bForceReselect)
		{
			NewTarget = BestByThreat;
		}
		else
		{
			const float CurrentThreat = ThreatComp->GetThreat(Current);
			if (BestThreat >= CurrentThreat * CurrentTargetThreatBonus + ThreatSwitchMargin)
			{
				NewTarget = BestByThreat;
			}
		}
	}
	else
	{
		// 5. 2순위 — 모든 후보의 위협도가 임계값 미만이면 최근접
		const FVector EnemyLoc = GetFocusLocationFor(EnemyPawn);
		APawn* Nearest = nullptr;
		float NearestDistSq = TNumericLimits<float>::Max();
		for (APawn* Candidate : Candidates)
		{
			const float DistSq = FVector::DistSquared(EnemyLoc, GetFocusLocationFor(Candidate));
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = Candidate;
			}
		}

		if (!Current || bForceReselect)
		{
			NewTarget = Nearest;
		}
		else if (Nearest)
		{
			const float CurrentDist = FVector::Dist(EnemyLoc, GetFocusLocationFor(Current));
			const float NearestDist = FMath::Sqrt(NearestDistSq);
			if (NearestDist + DistanceSwitchMargin < CurrentDist)
			{
				NewTarget = Nearest;
			}
		}
	}

	if (NewTarget != Current)
	{
		SetCurrentTarget(NewTarget);
		if (NewTarget)
		{
			SendAIEvent(MyTags::Souls::AIEvent_TargetChanged);
		}
	}
}
