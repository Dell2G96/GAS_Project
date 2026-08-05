// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LeeAttackTokenComponent.generated.h"

/** 어택 토큰 카테고리(Quota 태그) 하나의 상한 설정 */
USTRUCT(BlueprintType)
struct FLeeAttackSlotConfig
{
	GENERATED_BODY()

	/** 동시 공격 가능 인원 상한. 0 = 전면 거부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AttackToken", meta = (ClampMin = "0"))
	int32 MaxAttackers = 0;

	/** 동시 점유 가능 Cost 총합 상한. 0 = Cost 제한 비활성 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AttackToken", meta = (ClampMin = "0"))
	int32 MaxTotalCost = 0;
};

/** 공격 1종의 예약 정보 (Quota 태그 + Cost). Enemy 쪽 AttackerProfile이 보유한다 */
USTRUCT(BlueprintType)
struct FLeeAttackReservationConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AttackToken")
	FGameplayTag QuotaTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AttackToken", meta = (ClampMin = "1"))
	int32 Cost = 1;
};

/** 발급된 어택 토큰 예약 1건. ClaimId로 늦게 도착한 ExitState의 오반납을 방어한다 */
USTRUCT(BlueprintType)
struct FLeeAttackClaimHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Lee|AttackToken")
	FGuid ClaimId;

	UPROPERTY()
	TWeakObjectPtr<AActor> Requester;

	/** 실제로 점유한 모든 Quota 태그 (요청 태그 + 상위 조상 중 SlotConfigs에 등록된 것) */
	UPROPERTY()
	TArray<FGameplayTag> ConsumedQuotaTags;

	UPROPERTY(BlueprintReadOnly, Category = "Lee|AttackToken")
	int32 Cost = 0;

	bool IsValid() const { return ClaimId.IsValid(); }
};

/**
 * 플레이어(타겟) Pawn에 부착하는 어택 토큰 발급 컴포넌트.
 * Enemy AI가 공격을 시작하기 전 이 컴포넌트에서 토큰을 예약(TryClaim)하고,
 * 공격이 끝나면 반드시 반납(ReleaseByHandle/ReleaseAll)해 동시 공격 인원을 제한한다.
 * 요청 태그의 모든 상위 조상까지 누적 검사해 계층형 Quota 우회를 막는다.
 * 원본 패턴: ULeeTargetLockTargetComponent(타겟 부착형, static Find*, 화이트리스트 토글).
 */
UCLASS(Blueprintable, ClassGroup = (Lee), meta = (BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeAttackTokenComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULeeAttackTokenComponent();

	UFUNCTION(BlueprintPure, Category = "Lee|AttackToken")
	static ULeeAttackTokenComponent* FindAttackTokenComponent(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<ULeeAttackTokenComponent>() : nullptr;
	}

	/** Quota 태그별 상한 설정. 예: Souls.Attacker.Melee = {2, 2}, Souls.Attacker.Melee.Heavy = {1, 0} */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AttackToken")
	TMap<FGameplayTag, FLeeAttackSlotConfig> SlotConfigs;

	/** 반납 직후 같은 Requester가 다시 Claim할 수 없는 시간 (초). 0 = 비활성. TODO: 튜닝 필요 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AttackToken", meta = (ClampMin = "0.0"))
	float PerAttackerCooldown = 0.f;

	/** SlotConfigs에 등록되지 않은 카테고리를 거부할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AttackToken")
	bool bDenyUnconfiguredCategory = false;

	/** 보스 무적 페이즈 등에서 일시적으로 모든 Claim을 거부 (ULeeTargetLockTargetComponent::bCanBeLocked 패턴) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lee|AttackToken")
	bool bAcceptAttackTokens = true;

	/** [서버] 예약 가능 여부만 확인 (실제 점유 없음) */
	UFUNCTION(BlueprintCallable, Category = "Lee|AttackToken")
	bool CanClaim(const AActor* Requester, FGameplayTag QuotaTag, int32 Cost) const;

	/** [서버] Candidates 중 하나라도 Claim 가능하면 true (부모 State의 Enter Condition용) */
	UFUNCTION(BlueprintCallable, Category = "Lee|AttackToken")
	bool CanClaimAny(const AActor* Requester, const TArray<FLeeAttackReservationConfig>& Candidates) const;

	/** [서버] 토큰 예약 시도. 성공하면 OutHandle에 발급 정보를 채운다 */
	UFUNCTION(BlueprintCallable, Category = "Lee|AttackToken")
	bool TryClaim(AActor* Requester, FGameplayTag QuotaTag, int32 Cost, FLeeAttackClaimHandle& OutHandle);

	/** [서버] Handle의 ClaimId가 현재 저장된 엔트리와 일치할 때만 반납 (늦은 ExitState 방어) */
	UFUNCTION(BlueprintCallable, Category = "Lee|AttackToken")
	void ReleaseByHandle(const FLeeAttackClaimHandle& Handle);

	/** [서버] 특정 Requester의 모든 Claim을 반납 */
	UFUNCTION(BlueprintCallable, Category = "Lee|AttackToken")
	void ReleaseAll(AActor* Requester);

	UFUNCTION(BlueprintPure, Category = "Lee|AttackToken")
	bool HasAnyClaim(const AActor* Requester) const;

	UFUNCTION(BlueprintPure, Category = "Lee|AttackToken")
	int32 GetAttackerCount(FGameplayTag QuotaTag) const;

	UFUNCTION(BlueprintPure, Category = "Lee|AttackToken")
	int32 GetTotalCost(FGameplayTag QuotaTag) const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** CVar Lee.AI.DebugAttackTokens가 켜져 있을 때만 점유 현황을 머리 위에 문자열로 그린다 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Quota 태그 하나에 대한 점유 엔트리 1건 */
	struct FLeeClaimEntry
	{
		FGuid ClaimId;
		TWeakObjectPtr<AActor> Requester;
		int32 Cost = 0;
		double ClaimTime = 0.0;
	};

	/** Quota 태그별 점유 목록 */
	TMap<FGameplayTag, TArray<FLeeClaimEntry>> ClaimsByQuota;

	/** Requester별 마지막 반납 시각 (PerAttackerCooldown 판정용) */
	TMap<TWeakObjectPtr<AActor>, double> LastReleaseTime;

	/** OnEndPlay를 이미 바인딩한 Requester 집합 (중복 바인딩 방지) */
	TSet<TWeakObjectPtr<AActor>> EndPlayBoundRequesters;

	/** CanClaim/TryClaim이 공유하는 판정 로직. bCommit이 true일 때만 실제로 점유한다 */
	bool EvaluateClaim(AActor* Requester, FGameplayTag QuotaTag, int32 Cost, bool bCommit, FLeeAttackClaimHandle* OutHandle);

	/** 요청 태그 + 상위 조상 태그 중 SlotConfigs에 등록된 것만 추려 검사 대상으로 반환 */
	void ResolveQuotaTagsToCheck(FGameplayTag QuotaTag, TArray<FGameplayTag>& OutTags) const;

	/** 무효화된(파괴된) Requester의 점유 엔트리 정리 */
	void PruneInvalidRequesters();

	void BindEndPlayIfNeeded(AActor* Requester);
	void UnbindEndPlayIfNoClaims(AActor* Requester);

	UFUNCTION()
	void OnRequesterEndPlay(AActor* EndedActor, EEndPlayReason::Type EndPlayReason);

	/** 미등록 카테고리 허용 경고를 1회만 출력하기 위한 플래그 */
	static bool bHasWarnedUnconfiguredCategory;
};
