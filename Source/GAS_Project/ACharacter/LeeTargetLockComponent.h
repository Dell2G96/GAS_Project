// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "LeeTargetLockComponent.generated.h"

class ULeeCameraMode;
class UIndicatorDescriptor;
class UUserWidget;

/** 락온 상태 변경 UI 메시지 (GameplayMessageSubsystem, 채널: MyTags::Souls::Message_TargetLock) */
USTRUCT(BlueprintType)
struct FLeeTargetLockMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Lee|TargetLock")
	TObjectPtr<AActor> Instigator = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Lee|TargetLock")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Lee|TargetLock")
	bool bLocked = false;
};

/**
 * 타겟 락온의 모든 상태를 소유하는 컴포넌트 (플레이어 Pawn에 부착).
 *
 * 설계 원칙 (26.07.08 계획서/비교분석 하이브리드 채택안):
 *  - 락온은 수명이 긴 어빌리티가 아니라 이 컴포넌트가 상태를 소유한다.
 *    `ULeeGameplayAbility_TargetLock`은 ToggleLock()을 호출하고 즉시 종료되는 토글 스위치일 뿐이다.
 *  - 락온 카메라 모드(ULeeCameraMode_LockOn)가 View.ControlRotation을 타겟 방향으로 구동하면,
 *    LeeCameraComponent::GetCameraView()가 매 프레임 이 값을 PC에 적용 → 이동 입력 기준·캐릭터
 *    DesiredRotation이 자동으로 타겟 기준이 된다 (별도 회전 코드 불필요, 기존 파이프라인 재사용).
 *  - 카메라 우선순위: AbilityCameraMode(피니셔 등) > 락온 카메라 > 기본 카메라
 *    (ULeeHeroComponent::DetermineCameraMode 참고) — 피니셔 시작/종료가 락온을 몰라도
 *    카메라 전환/복귀가 자동으로 성립한다. 이 컴포넌트는 GA_Finisher를 직접 호출하지 않는다.
 *  - 팀 필터 필수: ULeeTeamSubsystem::CompareTeams로 아군(코옵 파트너) 제외.
 *  - 락온 후보는 ULeeTargetLockTargetComponent를 부착한 액터로 한정 (화이트리스트).
 */
UCLASS(Blueprintable, ClassGroup = (Lee), meta = (BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeTargetLockComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	ULeeTargetLockComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Lee|TargetLock")
	static ULeeTargetLockComponent* FindTargetLockComponent(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<ULeeTargetLockComponent>() : nullptr;
	}

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 락온 중이 아니면 시작 시도, 락온 중이면 해제. GA_TargetLock이 호출하는 유일한 진입점 */
	UFUNCTION(BlueprintCallable, Category = "Lee|TargetLock")
	bool ToggleLock();

	/** 어떤 이유로든 즉시 락온 해제 (대상 사망/거리 초과/시야 상실/수동 해제 공용) */
	UFUNCTION(BlueprintCallable, Category = "Lee|TargetLock")
	void BreakLock();

	/** 좌/우 전환. bWantRight=true면 화면 오른쪽의 가장 가까운 후보로 전환 */
	UFUNCTION(BlueprintCallable, Category = "Lee|TargetLock")
	bool SwitchTarget(bool bWantRight);

	UFUNCTION(BlueprintPure, Category = "Lee|TargetLock")
	bool IsLocked() const { return LockedTarget.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Lee|TargetLock")
	AActor* GetLockedTarget() const { return LockedTarget.Get(); }

	/** 카메라 모드가 바라볼 지점 (TargetLockTargetComponent 위치, 없으면 액터 위치로 대체) */
	UFUNCTION(BlueprintPure, Category = "Lee|TargetLock")
	FVector GetLockedTargetFocusLocation() const;

	/** 가드 스탠스: true = 왼발 뒤, false = 오른발 뒤(기본).
	 *  ABP(가드 포즈)와 GA_Guard(피격 몽타주 섹션)가 공유하는 단일 소스. */
	UFUNCTION(BlueprintPure, Category = "Lee|TargetLock")
	bool IsGuardLeftFootBack() const { return bGuardLeftFootBack; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** ULeeHeroComponent::DetermineCameraMode()가 락온 중일 때 반환할 카메라 모드 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lee|TargetLock")
	TSubclassOf<ULeeCameraMode> LockOnCameraMode;

protected:
	/** 락온 후보 스캔 반경 (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lee|TargetLock", meta = (ClampMin = "0.0"))
	float LockRadius = 2000.0f;

	/** 이 거리를 넘으면 자동 해제 (LockRadius보다 커야 함 — 히스테리시스로 경계에서 떨림 방지) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lee|TargetLock", meta = (ClampMin = "0.0"))
	float BreakRadius = 2500.0f;

	/** 최초 락온 시 카메라 시야 원뿔 절반각 (도) — 화면에 보이는 적만 후보로 인정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lee|TargetLock", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float ViewConeHalfAngleDeg = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lee|TargetLock")
	bool bRequireLineOfSight = true;

	/** 시야가 이 시간(초) 이상 끊기면 해제 (잠깐 기둥 뒤로 지나가는 것은 유지) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lee|TargetLock", meta = (ClampMin = "0.0"))
	float LineOfSightGraceTime = 0.75f;

	/** 좌/우 전환 연속 입력 방지 쿨다운 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lee|TargetLock", meta = (ClampMin = "0.0"))
	float SwitchCooldown = 0.3f;

	/** 락온 중 걷기 속도 제한. 0이면 변경하지 않음 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lee|TargetLock", meta = (ClampMin = "0.0"))
	float LockedMaxWalkSpeed = 0.0f;

	/** 가드 좌/우 전환 임계값(데드존). 이 값보다 확실히 넘을 때만 스탠스를 전환해 경계에서의 깜빡임을 막는다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lee|TargetLock|Guard", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GuardSideDeadzone = 0.2f;

	/** 락온 대상 UI 인디케이터 위젯 (ULeeIndicatorManagerComponent에 등록) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lee|TargetLock")
	TSubclassOf<UUserWidget> IndicatorWidgetClass;

private:
	bool TryStartLock();
	void StartLockOnActor(AActor* Target);

	void GatherCandidates(TArray<AActor*>& OutCandidates) const;
	bool IsValidCandidate(AActor* Candidate) const;
	bool PassesTeamFilter(AActor* Candidate) const;
	bool HasLineOfSight(const FVector& From, AActor* Candidate) const;
	FVector GetFocusLocationFor(AActor* Candidate) const;

	void CacheAndApplyMovementFlags();
	void RestoreMovementFlags();

	void ApplyStatusTag(bool bApply);
	void UpdateIndicator(bool bShow);
	void BroadcastLockMessage(bool bLocked) const;

	/** 락온 타겟 기준 좌/우 판정 — 소유 클라에서만 계산, 데드존 안에서는 이전 값 유지 */
	void UpdateGuardStance();

	/** 스탠스 확정 — 값이 바뀐 경우에만 서버로 전송 */
	void SetGuardLeftFootBack(bool bNewValue);

	/** 소유 클라 → 서버 동기화. 데드존 덕분에 호출 빈도가 매우 낮아 Reliable로 충분하다 */
	UFUNCTION(Server, Reliable)
	void Server_SetGuardLeftFootBack(bool bNewValue);

	/** 현재 락온 대상. nullptr이면 락온 중이 아님 */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> LockedTarget;

	/** 가드 스탠스. 소유 클라가 권위, 서버가 받아 시뮬레이트 프록시에 복제한다(COND_SkipOwner) */
	UPROPERTY(Replicated, Transient)
	bool bGuardLeftFootBack = false;

	UPROPERTY(Transient)
	TObjectPtr<UIndicatorDescriptor> LockIndicator;

	float LineOfSightLostElapsed = 0.0f;
	float SwitchCooldownRemaining = 0.0f;

	bool bMovementFlagsCached = false;
	bool bSavedOrientRotationToMovement = true;
	float CachedMaxWalkSpeed = 0.0f;
};
