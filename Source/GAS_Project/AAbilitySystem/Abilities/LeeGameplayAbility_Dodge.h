#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "LeeGameplayAbility_Dodge.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * 회피 어빌리티 — i-frame(무적) + 퍼펙트 회피 윈도우.
 *
 * i-frame 타이밍은 서버 타이머 기준이다 (IFrameStartTime/IFrameEndTime):
 * 데디 서버에서 AnimNotify가 실행되지 않아도 무적 시작/종료가 보장된다.
 * ULeeAnimNotifyState_GameplayEvent는 보조 신호일 뿐 판정에 사용하지 않는다.
 *
 * 흐름:
 *  1) CommitAbility (Cost GE = 스태미나 소모, BP에서 지정) → DodgeMontage 재생
 *  2) [서버] IFrameStartTime 타이머 → GE_DodgeInvincible(Infinite, 핸들 보관) + GE_PerfectDodgeWindow(Duration)
 *  3) [서버] IFrameEndTime 타이머 → 무적 GE 핸들 제거
 *  4) ExecCalc가 퍼펙트 회피 판정 → DefenseComponent가 Event.Defense.PerfectDodge 발송 → 수신 시
 *     GE_CounterWindow 적용 + 잔상 GameplayCue 실행 (리뷰 항목 3: WaitGameplayEvent 등록 포함)
 *  5) EndAbility/CancelAbility에서 무적/퍼펙트 윈도우 GE와 타이머를 무조건 정리 (태그 누수 방지)
 *
 * 무적 태그(Status.Invincible)는 GA_Finisher와 공유되므로 LooseTag 제거 금지 — 반드시 GE 핸들로만 제거 (리뷰 §6)
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_Dodge : public ULeeGameplayAbility
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_Dodge(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
	/** [서버] i-frame 시작 — 무적 GE + 퍼펙트 윈도우 GE 적용 */
	void OnIFrameBegin();

	/** [서버] i-frame 종료 — 무적 GE 핸들 제거 */
	void OnIFrameEnd();

	/** 퍼펙트 회피 이벤트 수신 → 반격 윈도우 + 잔상 GameplayCue */
	UFUNCTION()
	void OnPerfectDodgeEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	/** 무적/퍼펙트 윈도우 GE와 타이머 일괄 정리 (EndAbility 안전장치) */
	void CleanupDefenseWindows();

	/** SetByCaller Duration 방식 GE 적용 헬퍼 */
	FActiveGameplayEffectHandle ApplyDurationEffect(TSubclassOf<UGameplayEffect> EffectClass, float Duration);

	FName SelectDodgeSection() const;

	FVector GetDodgeInputVector() const;

	/** 무적 GE 핸들 — 반드시 핸들로만 제거 (LooseTag 제거 금지, Finisher 무적과 공유 태그) */
	FActiveGameplayEffectHandle InvincibleHandle;

	/** 퍼펙트 회피 윈도우 GE 핸들 */
	FActiveGameplayEffectHandle PerfectDodgeWindowHandle;

	FTimerHandle IFrameBeginTimerHandle;
	FTimerHandle IFrameEndTimerHandle;

protected:
	/** 회피 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Montage")
	TObjectPtr<UAnimMontage> DodgeMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Montage")
	FName DodgeForwardSection = TEXT("Forward");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Montage")
	FName DodgeBackwardSection = TEXT("Backward");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Montage")
	FName DodgeLeftSection = TEXT("Left");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Montage")
	FName DodgeRightSection = TEXT("Right");

	/** [서버 타이머] 몽타주 시작 후 i-frame 시작 시각 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge", meta = (ClampMin = "0.0"))
	float IFrameStartTime = 0.1f;

	/** [서버 타이머] 몽타주 시작 후 i-frame 종료 시각 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge", meta = (ClampMin = "0.0"))
	float IFrameEndTime = 0.5f;

	/** i-frame 시작 직후 퍼펙트 회피 판정 윈도우 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge", meta = (ClampMin = "0.0"))
	float PerfectDodgeWindow = 0.15f;

	/** 퍼펙트 회피 성공 시 반격 윈도우 지속시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge", meta = (ClampMin = "0.0"))
	float CounterWindowDuration = 0.5f;

	/** i-frame 무적 GE (Infinite, Status.Invincible 부여). BP에서 GE_DodgeInvincible 지정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Effect")
	TSubclassOf<UGameplayEffect> InvincibleEffect;

	/** 퍼펙트 회피 윈도우 GE. BP에서 GE_PerfectDodgeWindow 지정 (Duration = SetByCaller Souls.SetByCaller.Duration) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Effect")
	TSubclassOf<UGameplayEffect> PerfectDodgeWindowEffect;

	/** 반격 윈도우 GE. BP에서 GE_CounterWindow 지정 (Duration = SetByCaller Souls.SetByCaller.Duration) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Effect")
	TSubclassOf<UGameplayEffect> CounterWindowEffect;

};
