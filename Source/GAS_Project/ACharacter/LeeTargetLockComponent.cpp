// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeTargetLockComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACharacter/LeeTargetLockTargetComponent.h"
#include "GAS_Project/ATeam/LeeTeamSubsystem.h"
#include "GAS_Project/AUI/IndicatorDescriptor.h"
#include "GAS_Project/System/LeeIndicatorManagerComponent.h"
#include "Blueprint/UserWidget.h"
#include "Net/UnrealNetwork.h"

ULeeTargetLockComponent::ULeeTargetLockComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 락온 중에만 검사가 필요하므로 기본은 꺼두고 락온 시작 시에만 켠다
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// 가드 스탠스(bGuardLeftFootBack)를 시뮬레이트 프록시에 복제하기 위해 필요
	SetIsReplicatedByDefault(true);
}

// 가드 스탠스는 소유 클라가 권위이므로 되돌려 받지 않도록 COND_SkipOwner로 등록
void ULeeTargetLockComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ULeeTargetLockComponent, bGuardLeftFootBack, COND_SkipOwner);
}

void ULeeTargetLockComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Pawn 소멸/UnPossess 시 락온 상태·이동 플래그·UI가 남지 않도록 안전망
	BreakLock();

	Super::EndPlay(EndPlayReason);
}

void ULeeTargetLockComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (SwitchCooldownRemaining > 0.0f)
	{
		SwitchCooldownRemaining -= DeltaTime;
	}

	if (!IsLocked())
	{
		return;
	}

	AActor* Target = LockedTarget.Get();
	APawn* OwnerPawn = GetPawn<APawn>();
	if (!IsValid(Target) || !OwnerPawn)
	{
		BreakLock();
		return;
	}

	// 사망 검사
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC ||
		TargetASC->HasMatchingGameplayTag(MyTags::Lyra::Status_Death_Dying) ||
		TargetASC->HasMatchingGameplayTag(MyTags::Lyra::Status_Death_Dead))
	{
		BreakLock();
		return;
	}

	// 거리 검사 (히스테리시스 — BreakRadius는 LockRadius보다 크게 설정되어 있어야 경계에서 안 떨림)
	const float DistSq = FVector::DistSquared(OwnerPawn->GetActorLocation(), Target->GetActorLocation());
	if (DistSq > FMath::Square(BreakRadius))
	{
		BreakLock();
		return;
	}

	// 시야 검사 (유예 시간 기반 — 잠깐 기둥 뒤로 지나가는 것은 유지)
	if (bRequireLineOfSight)
	{
		if (const APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
		{
			FVector ViewLoc;
			FRotator ViewRot;
			PC->GetPlayerViewPoint(ViewLoc, ViewRot);

			if (!HasLineOfSight(ViewLoc, Target))
			{
				LineOfSightLostElapsed += DeltaTime;
				if (LineOfSightLostElapsed >= LineOfSightGraceTime)
				{
					BreakLock();
					return;
				}
			}
			else
			{
				LineOfSightLostElapsed = 0.0f;
			}
		}
	}

	// 가드 스탠스 갱신 — 여기까지 왔다면 락온·타겟 유효성이 모두 보장된 상태
	UpdateGuardStance();
}

bool ULeeTargetLockComponent::ToggleLock()
{
	if (IsLocked())
	{
		BreakLock();
		return true;
	}
	return TryStartLock();
}

bool ULeeTargetLockComponent::TryStartLock()
{
	APawn* OwnerPawn = GetPawn<APawn>();
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PC)
	{
		return false;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewLoc, ViewRot);
	const FVector ViewForward = ViewRot.Vector();
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(ViewConeHalfAngleDeg));

	TArray<AActor*> Candidates;
	GatherCandidates(Candidates);

	AActor* Best = nullptr;
	float BestScore = -1.0f;

	for (AActor* Candidate : Candidates)
	{
		const FVector ToCandidate = (GetFocusLocationFor(Candidate) - ViewLoc).GetSafeNormal();
		const float Dot = FVector::DotProduct(ViewForward, ToCandidate);
		if (Dot < MinDot)
		{
			continue;
		}

		if (bRequireLineOfSight && !HasLineOfSight(ViewLoc, Candidate))
		{
			continue;
		}

		// 화면 중앙에 가까울수록(Dot 높음) 우선, 락온 포인트의 Priority를 가산 (보스 우선)
		float Score = Dot;
		if (const ULeeTargetLockTargetComponent* TargetComp = ULeeTargetLockTargetComponent::FindTargetLockTargetComponent(Candidate))
		{
			Score += TargetComp->GetLockPriority();
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Candidate;
		}
	}

	if (!Best)
	{
		return false;
	}

	StartLockOnActor(Best);
	return true;
}

void ULeeTargetLockComponent::StartLockOnActor(AActor* Target)
{
	LockedTarget = Target;
	LineOfSightLostElapsed = 0.0f;
	SwitchCooldownRemaining = 0.0f;

	CacheAndApplyMovementFlags();
	ApplyStatusTag(true);
	UpdateIndicator(true);
	BroadcastLockMessage(true);

	SetComponentTickEnabled(true);
}

void ULeeTargetLockComponent::BreakLock()
{
	if (!IsLocked())
	{
		return;
	}

	RestoreMovementFlags();
	ApplyStatusTag(false);
	UpdateIndicator(false);
	BroadcastLockMessage(false);

	// 락온 해제 → 기본 스탠스(오른발 뒤)로 복귀. 이걸 빼면 락온을 풀어도 왼발 자세가 남는다
	SetGuardLeftFootBack(false);

	LockedTarget = nullptr;
	LineOfSightLostElapsed = 0.0f;
	SetComponentTickEnabled(false);
}

bool ULeeTargetLockComponent::SwitchTarget(bool bWantRight)
{
	if (!IsLocked() || SwitchCooldownRemaining > 0.0f)
	{
		return false;
	}

	APawn* OwnerPawn = GetPawn<APawn>();
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PC)
	{
		return false;
	}

	TArray<AActor*> Candidates;
	GatherCandidates(Candidates);
	Candidates.Remove(LockedTarget.Get());
	if (Candidates.Num() == 0)
	{
		return false;
	}

	FVector2D CurrentScreenPos;
	if (!PC->ProjectWorldLocationToScreen(GetFocusLocationFor(LockedTarget.Get()), CurrentScreenPos))
	{
		return false;
	}

	AActor* Best = nullptr;
	float BestDelta = TNumericLimits<float>::Max();

	for (AActor* Candidate : Candidates)
	{
		FVector2D ScreenPos;
		if (!PC->ProjectWorldLocationToScreen(GetFocusLocationFor(Candidate), ScreenPos))
		{
			continue;
		}

		const float DeltaX = ScreenPos.X - CurrentScreenPos.X;
		if (bWantRight ? (DeltaX <= 0.0f) : (DeltaX >= 0.0f))
		{
			continue;
		}

		const float AbsDelta = FMath::Abs(DeltaX);
		if (AbsDelta < BestDelta)
		{
			BestDelta = AbsDelta;
			Best = Candidate;
		}
	}

	if (!Best)
	{
		// 화면 좌/우에 후보가 없음 (Wrap은 1차 구현 범위 밖 — 필요 시 반대쪽 끝으로 확장)
		return false;
	}

	// 대상만 교체 — 이동 플래그/태그는 유지, 인디케이터만 갱신 (카메라는 RInterpTo로 자연스럽게 새 타겟으로 이동)
	UpdateIndicator(false);
	LockedTarget = Best;
	UpdateIndicator(true);
	BroadcastLockMessage(true);
	LineOfSightLostElapsed = 0.0f;
	SwitchCooldownRemaining = SwitchCooldown;
	return true;
}

FVector ULeeTargetLockComponent::GetLockedTargetFocusLocation() const
{
	return GetFocusLocationFor(LockedTarget.Get());
}

void ULeeTargetLockComponent::GatherCandidates(TArray<AActor*>& OutCandidates) const
{
	OutCandidates.Reset();

	const APawn* OwnerPawn = GetPawn<APawn>();
	UWorld* World = OwnerPawn ? OwnerPawn->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LeeTargetLock), /*bTraceComplex*/false, OwnerPawn);
	World->OverlapMultiByObjectType(
		Overlaps,
		OwnerPawn->GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(LockRadius),
		QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (Candidate && IsValidCandidate(Candidate))
		{
			OutCandidates.AddUnique(Candidate);
		}
	}
}

bool ULeeTargetLockComponent::IsValidCandidate(AActor* Candidate) const
{
	if (!Candidate || Candidate == GetPawn<APawn>())
	{
		return false;
	}

	// 락온 후보는 LeeTargetLockTargetComponent를 부착한 액터로 한정 (화이트리스트)
	const ULeeTargetLockTargetComponent* TargetComp = ULeeTargetLockTargetComponent::FindTargetLockTargetComponent(Candidate);
	if (!TargetComp || !TargetComp->CanBeLocked())
	{
		return false;
	}

	UAbilitySystemComponent* CandidateASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Candidate);
	if (!CandidateASC)
	{
		return false;
	}

	if (CandidateASC->HasMatchingGameplayTag(MyTags::Lyra::Status_Death_Dying) ||
		CandidateASC->HasMatchingGameplayTag(MyTags::Lyra::Status_Death_Dead))
	{
		return false;
	}

	if (!PassesTeamFilter(Candidate))
	{
		return false;
	}

	return true;
}

bool ULeeTargetLockComponent::PassesTeamFilter(AActor* Candidate) const
{
	const APawn* OwnerPawn = GetPawn<APawn>();
	const UWorld* World = OwnerPawn ? OwnerPawn->GetWorld() : nullptr;
	if (!OwnerPawn || !World)
	{
		return false;
	}

	if (const ULeeTeamSubsystem* TeamSubsystem = World->GetSubsystem<ULeeTeamSubsystem>())
	{
		const ELeeTeamComparison Comparison = TeamSubsystem->CompareTeams(OwnerPawn, Candidate);
		// 같은 팀(코옵 파트너 등)만 명시적으로 제외한다. 팀 미설정(InvalidArgument)은 허용 —
		// 팀 시스템이 아직 구성되지 않은 적에게도 락온이 동작해야 하기 때문이다 (Fail-open).
		return Comparison != ELeeTeamComparison::OnSameTeam;
	}

	return true;
}

bool ULeeTargetLockComponent::HasLineOfSight(const FVector& From, AActor* Candidate) const
{
	const UWorld* World = GetWorld();
	if (!World || !Candidate)
	{
		return true;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LeeTargetLockLOS), /*bTraceComplex*/false, GetPawn<APawn>());
	QueryParams.AddIgnoredActor(Candidate);

	FHitResult Hit;
	const bool bBlocked = World->LineTraceSingleByChannel(Hit, From, GetFocusLocationFor(Candidate), ECC_Visibility, QueryParams);
	return !bBlocked;
}

FVector ULeeTargetLockComponent::GetFocusLocationFor(AActor* Candidate) const
{
	if (const ULeeTargetLockTargetComponent* TargetComp = ULeeTargetLockTargetComponent::FindTargetLockTargetComponent(Candidate))
	{
		return TargetComp->GetFocusLocation();
	}
	return Candidate ? Candidate->GetActorLocation() : FVector::ZeroVector;
}

void ULeeTargetLockComponent::CacheAndApplyMovementFlags()
{
	if (ACharacter* Character = Cast<ACharacter>(GetPawn<APawn>()))
	{
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			bSavedOrientRotationToMovement = MoveComp->bOrientRotationToMovement;
			bMovementFlagsCached = true;

			// bUseControllerDesiredRotation은 건드리지 않는다 — 이미 true인 캐릭터라면
			// OrientRotationToMovement를 끄는 것만으로 ControlRotation(=락온 카메라가 매 프레임
			// 타겟 방향으로 채움) 기준 회전으로 자동 전환된다.
			MoveComp->bOrientRotationToMovement = false;

			if (LockedMaxWalkSpeed > 0.0f)
			{
				CachedMaxWalkSpeed = MoveComp->MaxWalkSpeed;
				MoveComp->MaxWalkSpeed = LockedMaxWalkSpeed;
			}
		}
	}
}

void ULeeTargetLockComponent::RestoreMovementFlags()
{
	if (!bMovementFlagsCached)
	{
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(GetPawn<APawn>()))
	{
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			MoveComp->bOrientRotationToMovement = bSavedOrientRotationToMovement;
			if (LockedMaxWalkSpeed > 0.0f)
			{
				MoveComp->MaxWalkSpeed = CachedMaxWalkSpeed;
			}
		}
	}

	bMovementFlagsCached = false;
}

void ULeeTargetLockComponent::ApplyStatusTag(bool bApply)
{
	APawn* OwnerPawn = GetPawn<APawn>();
	UAbilitySystemComponent* ASC = OwnerPawn ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerPawn) : nullptr;
	if (!ASC)
	{
		return;
	}

	if (bApply)
	{
		ASC->AddReplicatedLooseGameplayTag(MyTags::Souls::Status_TargetLock);
	}
	else
	{
		ASC->RemoveReplicatedLooseGameplayTag(MyTags::Souls::Status_TargetLock);
	}
}

void ULeeTargetLockComponent::UpdateIndicator(bool bShow)
{
	APawn* OwnerPawn = GetPawn<APawn>();
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PC)
	{
		return;
	}

	ULeeIndicatorManagerComponent* IndicatorManager = ULeeIndicatorManagerComponent::GetComponent(PC);
	if (!IndicatorManager)
	{
		return;
	}

	if (!bShow)
	{
		if (LockIndicator)
		{
			IndicatorManager->RemoveIndicator(LockIndicator);
			LockIndicator = nullptr;
		}
		return;
	}

	AActor* Target = LockedTarget.Get();
	if (!Target || !IndicatorWidgetClass)
	{
		return;
	}

	USceneComponent* AnchorComponent = ULeeTargetLockTargetComponent::FindTargetLockTargetComponent(Target);
	if (!AnchorComponent)
	{
		AnchorComponent = Target->GetRootComponent();
	}

	LockIndicator = NewObject<UIndicatorDescriptor>();
	LockIndicator->SetDataObject(Target);
	LockIndicator->SetSceneComponent(AnchorComponent);
	LockIndicator->SetIndicatorClass(TSoftClassPtr<UUserWidget>(IndicatorWidgetClass));
	IndicatorManager->AddIndicator(LockIndicator);
}

void ULeeTargetLockComponent::BroadcastLockMessage(bool bLocked) const
{
	const APawn* OwnerPawn = GetPawn<APawn>();
	if (!OwnerPawn)
	{
		return;
	}

	FLeeTargetLockMessage Message;
	Message.Instigator = const_cast<APawn*>(OwnerPawn);
	Message.Target = LockedTarget.Get();
	Message.bLocked = bLocked;

	UGameplayMessageSubsystem::Get(OwnerPawn->GetWorld()).BroadcastMessage(MyTags::Souls::Message_TargetLock, Message);
}

// 락온 타겟 기준 좌/우 판정 — 데드존 안에서는 이전 값을 유지해 경계에서 깜빡이지 않게 한다
void ULeeTargetLockComponent::UpdateGuardStance()
{
	const APawn* OwnerPawn = GetPawn<APawn>();
	const AActor* Target = LockedTarget.Get();
	if (!OwnerPawn || !Target || !OwnerPawn->IsLocallyControlled())
	{
		return; // 소유 클라(=화면을 보는 주체)만 계산한다. 서버/프록시는 동기화된 값을 쓴다
	}

	const FVector ToPlayer = (OwnerPawn->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
	const float SideDot = FVector::DotProduct(ToPlayer, Target->GetActorRightVector().GetSafeNormal2D());

	if (SideDot > GuardSideDeadzone)          // 타겟의 오른쪽 → 왼발 뒤
	{
		SetGuardLeftFootBack(true);
	}
	else if (SideDot < -GuardSideDeadzone)    // 타겟의 왼쪽 → 오른발 뒤
	{
		SetGuardLeftFootBack(false);
	}
	// 데드존 구간(-0.2 ~ +0.2)은 아무것도 하지 않음 = 이전 값 유지
}

// 스탠스 확정 — 값이 실제로 바뀐 경우에만 서버로 보낸다 (데드존 덕분에 전송이 매우 드물다)
void ULeeTargetLockComponent::SetGuardLeftFootBack(bool bNewValue)
{
	if (bGuardLeftFootBack == bNewValue)
	{
		return;
	}
	bGuardLeftFootBack = bNewValue;

	// 리슨서버 호스트는 이미 서버라 RPC가 필요 없다
	if (GetOwnerRole() != ROLE_Authority)
	{
		Server_SetGuardLeftFootBack(bNewValue);
	}
}

void ULeeTargetLockComponent::Server_SetGuardLeftFootBack_Implementation(bool bNewValue)
{
	bGuardLeftFootBack = bNewValue;
}
