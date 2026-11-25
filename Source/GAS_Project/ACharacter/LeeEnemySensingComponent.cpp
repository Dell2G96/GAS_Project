// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeEnemySensingComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/LeeLogChannels.h"

// 기억 중인 플레이어를 시야 판정에서 놓칠 때마다 실패 사유를 로그로 남기는 진단 스위치 (0 = 끔, 1 = 켬)
static TAutoConsoleVariable<int32> CVarLeeDebugSensing(
	TEXT("Lee.AI.DebugSensing"),
	0,
	TEXT("1이면 Enemy가 기억 중인 플레이어를 시야 판정에서 놓칠 때마다 사유(거리/각도/가림)를 로그로 남긴다."),
	ECVF_Cheat);

ULeeEnemySensingComponent::ULeeEnemySensingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULeeEnemySensingComponent::BeginPlay()
{
	Super::BeginPlay();

	// 판정은 서버 전용. 태그는 리플리케이트로 클라이언트에 전달된다.
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	// 스폰 시 미인식 상태로 시작 (ASC 초기화 순서 때문에 다음 틱에 적용)
	GetWorld()->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			SetUnaware(true);
		}));

	GetWorld()->GetTimerManager().SetTimer(SenseTimerHandle, this, &ThisClass::SenseTick, SenseInterval, /*bLoop*/true);
}

void ULeeEnemySensingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SenseTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ULeeEnemySensingComponent::SenseTick()
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		
		return;
	}

	// 피니셔 당하는 중이거나 사망 시 인식 상태를 건드리지 않는다
	if (ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Finisher_Victim) ||
		ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Death_Dying) ||
		ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Death_Dead))
	{
	
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();

	bool bPerceivedAny = false;
	PerceivedPawns.Reset();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
		// 기억에 남아 있으면 이미 전투 중인 대상 → 완화된 조건으로 판정
		const bool bAlreadyAware = PlayerPawn && LastSeenTimeByPawn.Contains(PlayerPawn);
		FString FailReason;
		const bool bCanPerceive = PlayerPawn && CanPerceive(PlayerPawn, bAlreadyAware, &FailReason);

		if (bCanPerceive)
		{
			bPerceivedAny = true;
			LastSeenTimeByPawn.Add(PlayerPawn, Now);
			LastFailReasonByPawn.Remove(PlayerPawn);
			PerceivedPawns.Add(PlayerPawn);
		}
		else if (PlayerPawn)
		{
			// 시야를 벗어나도 LoseSightDelay 동안은 "기억"으로 후보에 남긴다
			// (전투 중 플레이어가 등 뒤로 돌면 즉시 타겟을 잃던 문제 해결)
			const double* LastSeen = LastSeenTimeByPawn.Find(PlayerPawn);
			if (LastSeen && (Now - *LastSeen) <= LoseSightDelay)
			{
				PerceivedPawns.Add(PlayerPawn);

				// [진단] 기억 중인 플레이어를 놓친 사유를 남긴다
				LastFailReasonByPawn.Add(PlayerPawn, FailReason);
				if (CVarLeeDebugSensing.GetValueOnGameThread() != 0)
				{
					UE_LOG(LogLee, Log, TEXT("[Sensing] %s → %s 시야 실패: %s (남은 기억 %.1f초)"),
						*GetNameSafe(GetOwner()), *GetNameSafe(PlayerPawn), *FailReason,
						LoseSightDelay - (Now - *LastSeen));
				}
			}
		}
	}

	// 유예 시간이 지났거나 무효화된 기억 항목을 정리
	for (TMap<TWeakObjectPtr<APawn>, double>::TIterator It(LastSeenTimeByPawn); It; ++It)
	{
		if (!It->Key.IsValid() || (Now - It->Value) > LoseSightDelay)
		{
			// [진단] 플레이어를 완전히 잊는 순간은 드물고 중요하므로 CVar와 관계없이 항상 남긴다
			if (APawn* ForgottenPawn = It->Key.Get())
			{
				const FString* LastReason = LastFailReasonByPawn.Find(ForgottenPawn);
				UE_LOG(LogLee, Warning, TEXT("[Sensing] %s가 %s를 잊음 (%.1f초 동안 못 봄) — 마지막 실패 사유: %s"),
					*GetNameSafe(GetOwner()), *GetNameSafe(ForgottenPawn), LoseSightDelay,
					LastReason ? **LastReason : TEXT("알 수 없음"));
			}
			LastFailReasonByPawn.Remove(It->Key);
			It.RemoveCurrent();
		}
	}

	if (bPerceivedAny)
	{
		LastPerceivedTime = Now;
		if (bUnaware)
		{
			// 인식 성공 → 즉시 암살 불가 상태로 전환 (UI는 다음 판정 주기에 숨겨짐)
			SetUnaware(false);
		}
	}
	else if (!bUnaware && (Now - LastPerceivedTime) > LoseSightDelay)
	{
		// 전투 이탈 → 다시 암살 가능 상태로 복귀
		SetUnaware(true);
	}
}

// 거리 → 전방 시야각 → 차폐 순으로 검사. 이미 전투 중인 대상은 조건을 완화한다. 실패 사유는 OutFailReason에 적는다(진단 로그용)
bool ULeeEnemySensingComponent::CanPerceive(const APawn* PlayerPawn, bool bAlreadyAware, FString* OutFailReason) const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !PlayerPawn)
	{
		return false;
	}

	const FVector OwnerLocation = Owner->GetActorLocation();
	const FVector PlayerLocation = PlayerPawn->GetActorLocation();

	// ① 거리 — 처음 발견은 SightRadius, 전투 중에는 CombatLoseSightRadius까지 유지
	const float Distance = FVector::Dist(OwnerLocation, PlayerLocation);
	const float MaxDistance = bAlreadyAware ? CombatLoseSightRadius : SightRadius;
	if (Distance > MaxDistance)
	{
		if (OutFailReason)
		{
			*OutFailReason = FString::Printf(TEXT("거리 %.0fcm > %.0fcm"), Distance, MaxDistance);
		}
		return false;
	}

	// 전투 중 + 가까움 → 시야각·가림과 관계없이 인지 (바로 옆 플레이어를 장애물 모서리 때문에 잊지 않게)
	if (bAlreadyAware && Distance <= CloseAwarenessRadius)
	{
		return true;
	}

	// ② 전방 시야각 — 처음 발견할 때만 검사 (전투 중에는 옆·뒤로 돌아가도 유지)
	if (!bAlreadyAware)
	{
		const FVector OwnerForward = Owner->GetActorForwardVector().GetSafeNormal2D();
		const FVector ToPlayer = (PlayerLocation - OwnerLocation).GetSafeNormal2D();
		const float FrontDotThreshold = FMath::Cos(FMath::DegreesToRadians(SightAngleDeg * 0.5f));
		const float FrontDot = FVector::DotProduct(OwnerForward, ToPlayer);
		if (FrontDot < FrontDotThreshold)
		{
			if (OutFailReason)
			{
				const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FrontDot, -1.f, 1.f)));
				*OutFailReason = FString::Printf(TEXT("시야각 밖 %.0f° > %.0f°"), AngleDeg, SightAngleDeg * 0.5f);
			}
			return false;
		}
	}

	// ③ 차폐 검사 (벽·지형만 시야를 가린다)
	if (bCheckLineOfSight)
	{
		FString Blocker;
		if (!HasLineOfSight(PlayerPawn, &Blocker))
		{
			if (OutFailReason)
			{
				*OutFailReason = FString::Printf(TEXT("시야 가림 %s"), *Blocker);
			}
			return false;
		}
	}

	return true;
}

// 눈높이 → 플레이어 중심으로 Visibility 트레이스. 다른 폰(동료 Enemy 등)에 맞으면 무시 목록에 넣고 다시 쏜다
bool ULeeEnemySensingComponent::HasLineOfSight(const APawn* PlayerPawn, FString* OutBlocker) const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !PlayerPawn)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LeeEnemySensing), /*bTraceComplex*/false, Owner);
	QueryParams.AddIgnoredActor(PlayerPawn);

	const FVector EyeLocation = Owner->GetActorLocation() + FVector(0.0f, 0.0f, EyeHeightOffset);
	const FVector PlayerLocation = PlayerPawn->GetActorLocation();

	// 폰이 줄지어 서 있어도 몇 번이면 충분하다 (무한 반복 방지용 상한)
	constexpr int32 MaxPawnSkips = 4;
	for (int32 Attempt = 0; Attempt <= MaxPawnSkips; ++Attempt)
	{
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, PlayerLocation, ECC_Visibility, QueryParams))
		{
			return true;
		}

		AActor* HitActor = Hit.GetActor();
		if (Cast<APawn>(HitActor))
		{
			QueryParams.AddIgnoredActor(HitActor);
			continue;
		}

		if (OutBlocker)
		{
			*OutBlocker = FString::Printf(TEXT("%s (%s)"), *GetNameSafe(HitActor), *GetNameSafe(Hit.GetComponent()));
		}
		return false;
	}

	// 폰만 계속 맞은 경우 — 벽·지형은 없었으므로 보인 것으로 본다
	return true;
}

// [서버] 공격당하면 시야와 관계없이 공격자를 인지 목록·기억에 넣고 미인식 상태를 해제한다
void ULeeEnemySensingComponent::NotifyAttackedBy(APawn* Attacker)
{
	if (GetOwnerRole() != ROLE_Authority || !Attacker || !Attacker->IsPlayerControlled())
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	LastSeenTimeByPawn.Add(Attacker, Now);
	LastFailReasonByPawn.Remove(Attacker);
	PerceivedPawns.AddUnique(Attacker);
	LastPerceivedTime = Now;

	if (bUnaware)
	{
		SetUnaware(false);
	}
}

void ULeeEnemySensingComponent::SetUnaware(bool bNewUnaware)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	bUnaware = bNewUnaware;

	// 서버 로컬 카운트 + 리플리케이트 양쪽 반영 — 클라이언트의 프롬프트 판정에서도 태그가 보여야 함
	if (bNewUnaware)
	{
		if (!ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Unaware))
		{
			ASC->AddLooseGameplayTag(MyTags::Souls::Status_Unaware);
			ASC->AddReplicatedLooseGameplayTag(MyTags::Souls::Status_Unaware);
		}
	}
	else
	{
		if (ASC->HasMatchingGameplayTag(MyTags::Souls::Status_Unaware))
		{
			ASC->RemoveLooseGameplayTag(MyTags::Souls::Status_Unaware);
			ASC->RemoveReplicatedLooseGameplayTag(MyTags::Souls::Status_Unaware);
		}
	}
}

UAbilitySystemComponent* ULeeEnemySensingComponent::GetOwnerASC() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

// 직전 SenseTick에서 CanPerceive를 통과한 플레이어 폰 목록을 반환 (무효화된 항목은 제외)
void ULeeEnemySensingComponent::GetPerceivedPawns(TArray<APawn*>& OutPawns) const
{
	OutPawns.Reset();
	for (const TWeakObjectPtr<APawn>& WeakPawn : PerceivedPawns)
	{
		if (APawn* Pawn = WeakPawn.Get())
		{
			OutPawns.Add(Pawn);
		}
	}
}
