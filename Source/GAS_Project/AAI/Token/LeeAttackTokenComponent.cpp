// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeAttackTokenComponent.h"
#include "GAS_Project/LeeLogChannels.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

bool ULeeAttackTokenComponent::bHasWarnedUnconfiguredCategory = false;

// 어택 토큰 점유 현황을 타겟 머리 위에 표시하는 디버그 스위치 (0 = 끔, 1 = 켬)
static TAutoConsoleVariable<int32> CVarLeeDebugAttackTokens(
	TEXT("Lee.AI.DebugAttackTokens"),
	0,
	TEXT("1이면 어택 토큰 Quota별 점유수/상한, 코스트합/상한을 타겟 머리 위에 표시한다."),
	ECVF_Cheat);

// 어택 토큰 발급/반납을 관리하는 컴포넌트 생성자. 판정은 서버 전용이며, 틱은 디버그 표시에만 사용한다
ULeeAttackTokenComponent::ULeeAttackTokenComponent()
{
	// 디버그 CVar가 꺼져 있으면 TickComponent가 즉시 반환하므로 상시 틱이어도 비용이 거의 없다
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(false);
}

// CVar가 켜져 있을 때만 Quota별 점유 현황을 문자열로 그린다 (계획서 §4-1 디버그 항목)
void ULeeAttackTokenComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !UE_BUILD_SHIPPING
	if (CVarLeeDebugAttackTokens.GetValueOnGameThread() == 0)
	{
		return;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	FString DebugText = FString::Printf(TEXT("[AttackTokens] %s%s\n"),
		*Owner->GetName(), bAcceptAttackTokens ? TEXT("") : TEXT(" (거부중)"));

	for (const TPair<FGameplayTag, FLeeAttackSlotConfig>& Slot : SlotConfigs)
	{
		DebugText += FString::Printf(TEXT("%s : %d/%d명, Cost %d/%s\n"),
			*Slot.Key.ToString(),
			GetAttackerCount(Slot.Key),
			Slot.Value.MaxAttackers,
			GetTotalCost(Slot.Key),
			Slot.Value.MaxTotalCost > 0 ? *FString::FromInt(Slot.Value.MaxTotalCost) : TEXT("무제한"));
	}

	const FVector DrawLocation = Owner->GetActorLocation() + FVector(0.f, 0.f, 140.f);
	DrawDebugString(GetWorld(), DrawLocation, DebugText, nullptr, FColor::Yellow, 0.f, /*bDrawShadow*/true);
#endif
}

// EndPlay 시 보관 중인 모든 Claim과 OnEndPlay 바인딩을 정리 (회수 경로 3: 컴포넌트/오너 종료)
void ULeeAttackTokenComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const TWeakObjectPtr<AActor>& WeakRequester : EndPlayBoundRequesters)
	{
		if (AActor* Requester = WeakRequester.Get())
		{
			Requester->OnEndPlay.RemoveDynamic(this, &ULeeAttackTokenComponent::OnRequesterEndPlay);
		}
	}
	EndPlayBoundRequesters.Empty();
	ClaimsByQuota.Empty();
	LastReleaseTime.Empty();

	Super::EndPlay(EndPlayReason);
}

// 요청 태그와 그 모든 상위 조상 태그 중 SlotConfigs에 등록된 것만 추려 검사 대상으로 반환 (P0-1 계층 Quota)
void ULeeAttackTokenComponent::ResolveQuotaTagsToCheck(FGameplayTag QuotaTag, TArray<FGameplayTag>& OutTags) const
{
	OutTags.Reset();
	const FGameplayTagContainer Ancestors = QuotaTag.GetGameplayTagParents();
	for (const FGameplayTag& Tag : Ancestors)
	{
		if (SlotConfigs.Contains(Tag))
		{
			OutTags.Add(Tag);
		}
	}
}

// 무효화된(파괴된) Requester의 점유 엔트리를 정리
void ULeeAttackTokenComponent::PruneInvalidRequesters()
{
	for (TPair<FGameplayTag, TArray<FLeeClaimEntry>>& Pair : ClaimsByQuota)
	{
		Pair.Value.RemoveAll([](const FLeeClaimEntry& Entry)
		{
			return !Entry.Requester.IsValid();
		});
	}
}

// CanClaim/TryClaim이 공유하는 판정 로직 (P0-2 판정 규칙). bCommit이 true일 때만 실제로 점유한다
bool ULeeAttackTokenComponent::EvaluateClaim(AActor* Requester, FGameplayTag QuotaTag, int32 Cost, bool bCommit, FLeeAttackClaimHandle* OutHandle)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return false;
	}

	if (!bAcceptAttackTokens || !Requester || !QuotaTag.IsValid())
	{
		return false;
	}

	PruneInvalidRequesters();

	if (!ensureMsgf(Cost > 0, TEXT("ULeeAttackTokenComponent: Cost는 0보다 커야 합니다 (%s)"), *QuotaTag.ToString()))
	{
		return false;
	}

	if (PerAttackerCooldown > 0.f)
	{
		if (const double* LastRelease = LastReleaseTime.Find(Requester))
		{
			const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
			if (Now - *LastRelease < PerAttackerCooldown)
			{
				return false;
			}
		}
	}

	TArray<FGameplayTag> TagsToCheck;
	ResolveQuotaTagsToCheck(QuotaTag, TagsToCheck);

	if (TagsToCheck.Num() == 0)
	{
		if (bDenyUnconfiguredCategory)
		{
			return false;
		}

		if (!bHasWarnedUnconfiguredCategory)
		{
			bHasWarnedUnconfiguredCategory = true;
			UE_LOG(LogLee, Warning, TEXT("ULeeAttackTokenComponent: '%s'가 SlotConfigs에 등록되지 않아 제한 없이 허용합니다."), *QuotaTag.ToString());
		}
	}
	else
	{
		for (const FGameplayTag& Tag : TagsToCheck)
		{
			const FLeeAttackSlotConfig& Config = SlotConfigs[Tag];
			if (Config.MaxAttackers == 0)
			{
				return false;
			}
			if (GetAttackerCount(Tag) + 1 > Config.MaxAttackers)
			{
				return false;
			}
			if (Config.MaxTotalCost > 0 && GetTotalCost(Tag) + Cost > Config.MaxTotalCost)
			{
				return false;
			}
		}
	}

	if (!bCommit)
	{
		return true;
	}

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	FLeeClaimEntry NewEntry;
	NewEntry.ClaimId = FGuid::NewGuid();
	NewEntry.Requester = Requester;
	NewEntry.Cost = Cost;
	NewEntry.ClaimTime = Now;

	for (const FGameplayTag& Tag : TagsToCheck)
	{
		ClaimsByQuota.FindOrAdd(Tag).Add(NewEntry);
	}

	BindEndPlayIfNeeded(Requester);

	if (OutHandle)
	{
		OutHandle->ClaimId = NewEntry.ClaimId;
		OutHandle->Requester = Requester;
		OutHandle->ConsumedQuotaTags = TagsToCheck;
		OutHandle->Cost = Cost;
	}

	return true;
}

// [서버] 예약 가능 여부만 확인 (실제 점유 없음)
bool ULeeAttackTokenComponent::CanClaim(const AActor* Requester, FGameplayTag QuotaTag, int32 Cost) const
{
	return const_cast<ULeeAttackTokenComponent*>(this)->EvaluateClaim(const_cast<AActor*>(Requester), QuotaTag, Cost, /*bCommit=*/false, nullptr);
}

// [서버] Candidates 중 하나라도 Claim 가능하면 true
bool ULeeAttackTokenComponent::CanClaimAny(const AActor* Requester, const TArray<FLeeAttackReservationConfig>& Candidates) const
{
	for (const FLeeAttackReservationConfig& Candidate : Candidates)
	{
		if (CanClaim(Requester, Candidate.QuotaTag, Candidate.Cost))
		{
			return true;
		}
	}
	return false;
}

// [서버] 토큰 예약 시도
bool ULeeAttackTokenComponent::TryClaim(AActor* Requester, FGameplayTag QuotaTag, int32 Cost, FLeeAttackClaimHandle& OutHandle)
{
	if (HasAnyClaim(Requester))
	{
		UE_LOG(LogLee, Warning, TEXT("ULeeAttackTokenComponent: %s가 이미 Claim을 보유한 상태에서 새 Claim을 요청했습니다."), *GetNameSafe(Requester));
	}

	return EvaluateClaim(Requester, QuotaTag, Cost, /*bCommit=*/true, &OutHandle);
}

// [서버] Handle의 ClaimId가 현재 저장된 엔트리와 일치할 때만 반납 (늦은 ExitState 방어, P0-5 회수 경로 1)
void ULeeAttackTokenComponent::ReleaseByHandle(const FLeeAttackClaimHandle& Handle)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !Handle.IsValid())
	{
		return;
	}

	bool bRemovedAny = false;
	for (const FGameplayTag& Tag : Handle.ConsumedQuotaTags)
	{
		if (TArray<FLeeClaimEntry>* Entries = ClaimsByQuota.Find(Tag))
		{
			const int32 RemovedCount = Entries->RemoveAll([&Handle](const FLeeClaimEntry& Entry)
			{
				return Entry.ClaimId == Handle.ClaimId;
			});
			bRemovedAny |= (RemovedCount > 0);
		}
	}

	if (bRemovedAny && Handle.Requester.IsValid())
	{
		AActor* Requester = Handle.Requester.Get();
		const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
		LastReleaseTime.Add(Requester, Now);
		UnbindEndPlayIfNoClaims(Requester);
	}
}

// [서버] 특정 Requester의 모든 Claim을 전부 반납 (회수 경로 2/4/5: 사망·타겟변경·UnPossess 등에서 호출)
void ULeeAttackTokenComponent::ReleaseAll(AActor* Requester)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !Requester)
	{
		return;
	}

	bool bRemovedAny = false;
	for (TPair<FGameplayTag, TArray<FLeeClaimEntry>>& Pair : ClaimsByQuota)
	{
		const int32 RemovedCount = Pair.Value.RemoveAll([Requester](const FLeeClaimEntry& Entry)
		{
			return Entry.Requester.Get() == Requester;
		});
		bRemovedAny |= (RemovedCount > 0);
	}

	if (bRemovedAny)
	{
		const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
		LastReleaseTime.Add(Requester, Now);
	}

	UnbindEndPlayIfNoClaims(Requester);
}

// 특정 Requester가 이 컴포넌트에 하나 이상의 Claim을 보유 중인지 확인
bool ULeeAttackTokenComponent::HasAnyClaim(const AActor* Requester) const
{
	if (!Requester)
	{
		return false;
	}

	for (const TPair<FGameplayTag, TArray<FLeeClaimEntry>>& Pair : ClaimsByQuota)
	{
		for (const FLeeClaimEntry& Entry : Pair.Value)
		{
			if (Entry.Requester.Get() == Requester)
			{
				return true;
			}
		}
	}
	return false;
}

// 특정 Quota 태그를 점유 중인 Requester 수
int32 ULeeAttackTokenComponent::GetAttackerCount(FGameplayTag QuotaTag) const
{
	if (const TArray<FLeeClaimEntry>* Entries = ClaimsByQuota.Find(QuotaTag))
	{
		return Entries->Num();
	}
	return 0;
}

// 특정 Quota 태그의 점유 Cost 총합
int32 ULeeAttackTokenComponent::GetTotalCost(FGameplayTag QuotaTag) const
{
	int32 Total = 0;
	if (const TArray<FLeeClaimEntry>* Entries = ClaimsByQuota.Find(QuotaTag))
	{
		for (const FLeeClaimEntry& Entry : *Entries)
		{
			Total += Entry.Cost;
		}
	}
	return Total;
}

// Requester의 OnEndPlay를 아직 바인딩하지 않았다면 바인딩 (Requester당 1회만, 회수 경로 6)
void ULeeAttackTokenComponent::BindEndPlayIfNeeded(AActor* Requester)
{
	if (!Requester || EndPlayBoundRequesters.Contains(Requester))
	{
		return;
	}

	Requester->OnEndPlay.AddDynamic(this, &ULeeAttackTokenComponent::OnRequesterEndPlay);
	EndPlayBoundRequesters.Add(Requester);
}

// 마지막 Claim이 사라지면 OnEndPlay 바인딩 해제
void ULeeAttackTokenComponent::UnbindEndPlayIfNoClaims(AActor* Requester)
{
	if (!Requester || HasAnyClaim(Requester))
	{
		return;
	}

	if (EndPlayBoundRequesters.Contains(Requester))
	{
		Requester->OnEndPlay.RemoveDynamic(this, &ULeeAttackTokenComponent::OnRequesterEndPlay);
		EndPlayBoundRequesters.Remove(Requester);
	}
}

// Requester가 EndPlay되면 보유 중인 모든 Claim을 강제로 반납
void ULeeAttackTokenComponent::OnRequesterEndPlay(AActor* EndedActor, EEndPlayReason::Type EndPlayReason)
{
	ReleaseAll(EndedActor);
}
