// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "LeeGameplayAbility_Guard.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * 가드 어빌리티 (Hold — 입력 유지 중 활성).
 *
 * 흐름:
 *  1) 활성화 직후 GE_PerfectGuardWindow 적용 (SetByCaller Duration = PerfectGuardWindow)
 *     → Souls.Status.Guard.Perfect 태그가 짧게 유지되다 자동 만료 (퍼펙트 가드 판정 윈도우)
 *  2) Souls.Status.Guard.Active는 ActivationOwnedTags로 자동 부여/제거 → ABP GuardState 전환용
 *  3) ExecCalc 판정 결과를 DefenseComponent가 이벤트로 쏘면 수신:
 *     - Event.Defense.GuardHit      → 플린치 몽타주 (가드 유지)
 *     - Event.Defense.PerfectGuard  → 패리 몽타주 + GE_CounterWindow 적용
 *  4) 입력 해제(WaitInputRelease) → EndAbility. 퍼펙트 윈도우 GE 핸들은 EndAbility에서 반드시 제거
 *     (가드 온오프 반복 시 이전 윈도우 GE 중복 잔류 방지 )
 *
 * ActivationGroup = Independent — 공격 차단은 공격 어빌리티 쪽 ActivationBlockedTags(Guard.Active)로 처리
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_Guard : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_Guard(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;
	
	
private:
	/** 일반 가드 피격 이벤트 수신 → 플린치 몽타주 재생 (가드 유지) */
	UFUNCTION()
	void OnGuardHitEventReceived(FGameplayEventData Payload);

	/** 퍼펙트 가드 이벤트 수신 → 패리 몽타주 + 반격 윈도우 GE 적용 */
	UFUNCTION()
	void OnPerfectGuardEventReceived(FGameplayEventData Payload);

	/** 가드 입력 해제 → 어빌리티 종료 */
	UFUNCTION()
	void OnInputReleased(float TimeHeld);

	/** SetByCaller Duration 방식 GE 적용 헬퍼 */
	FActiveGameplayEffectHandle ApplyDurationEffect(TSubclassOf<UGameplayEffect> EffectClass, float Duration);

	/** 현재 가드 스탠스(왼발 뒤/오른발 뒤)에 맞는 몽타주 섹션 이름 선택. 섹션이 없으면 NAME_None으로 안전 폴백 */
	FName SelectStanceSection(const UAnimMontage* Montage) const;

	/** 몽타주 + 스탠스 섹션 재생 공통 헬퍼 (플린치/패리 공용) */
	void PlayGuardMontageWithStance(UAnimMontage* Montage);

	/** 퍼펙트 가드 윈도우 GE 핸들 — EndAbility에서 반드시 제거*/
	FActiveGameplayEffectHandle PerfectGuardWindowHandle;

protected:
	/** 가드 시작 후 퍼펙트 판정 윈도우 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Guard", meta = (ClampMin = "0.0"))
	float PerfectGuardWindow = 0.2f;

	/** 퍼펙트 가드 성공 시 반격 윈도우 지속시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Guard", meta = (ClampMin = "0.0"))
	float CounterWindowDuration = 0.5f;

	/** 일반 가드 피격 플린치 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Guard|Montage")
	TObjectPtr<UAnimMontage> GuardFlinchMontage;

	/** 퍼펙트 가드(패리) 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Guard|Montage")
	TObjectPtr<UAnimMontage> GuardParryMontage;

	/** 왼발 뒤 스탠스 피격/패리 섹션 이름 (두 몽타주 공통 규약) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Guard|Montage")
	FName StanceLeftFootSection = TEXT("LeftFootBack");

	/** 오른발 뒤(기본) 스탠스 섹션 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Guard|Montage")
	FName StanceRightFootSection = TEXT("RightFootBack");

	/** 퍼펙트 가드 윈도우 GE. BP에서 GE_PerfectGuardWindow 지정 (Duration = SetByCaller Souls.SetByCaller.Duration) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Guard|Effect")
	TSubclassOf<UGameplayEffect> PerfectGuardWindowEffect;

	/** 반격 윈도우 GE. BP에서 GE_CounterWindow 지정 (Duration = SetByCaller Souls.SetByCaller.Duration) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Guard|Effect")
	TSubclassOf<UGameplayEffect> CounterWindowEffect;

};
