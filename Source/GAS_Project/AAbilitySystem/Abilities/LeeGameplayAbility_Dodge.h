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
 * 26.11.13 - 더블탭 구르기: 첫 입력 후 DoubleTapWindow 동안 "뗐다가 다시 누름"을 기다린다.
 *  다시 누르면 RollMontage(구르기), 시간이 지나면 DodgeMontage(스텝). 결정은 로컬 조작자가 하고,
 *  게스트는 GameCustom1(스텝)/GameCustom2(구르기) 신호로 서버에 알린다 (LocalPredicted).
 *
 * 흐름:
 *  1) CommitAbility (Cost GE = 스태미나 소모, BP에서 지정) → 스텝/구르기 결정 → 몽타주 재생
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
	// ── 26.11.13 추가 - 더블탭 구르기 ─────────────────────────────
	/** [로컬 조작자] 스페이스를 한 번 뗀 것 확인 → 두 번째 누름 대기 시작 */
	UFUNCTION()
	void OnFirstInputReleased(float TimeHeld);

	/** [로컬 조작자] 대기 시간 안에 두 번째 누름 → 구르기 확정 */
	UFUNCTION()
	void OnSecondInputPressed(float TimeWaited);

	/** [로컬 조작자] 대기 시간 만료 → 스텝 확정 */
	void OnDoubleTapWindowExpired();

	/** [로컬 조작자] 결정 결과로 동작 시작 + (게스트면) 서버에 결정 전송 */
	void DecideDodgeLocally(bool bRoll);

	/** [서버, 원격 플레이어] 게스트가 보낸 스텝/구르기 결정 신호 구독 */
	void ListenForRemoteDecision();

	/** [서버, 원격 플레이어] 결정 신호 수신 */
	void OnRemoteStepDecided();
	void OnRemoteRollDecided();

	/** 스텝 또는 구르기 몽타주 재생 + 서버 i-frame 타이머 예약 (결정 후 1회만) */
	void StartDodgeMove(bool bRoll);

	/** 구르기 가능 여부 — RollMontage가 있고 추가 스태미나를 낼 수 있어야 한다 */
	bool CanRoll() const;

	/** 구르기 직전 입력 방향으로 캐릭터를 돌린다 (구르기는 한 방향 몽타주) */
	void RotateToDodgeDirection();

	bool IsServerForRemotePlayer() const;

	/** 결정 대기 타이머 / 서버 안전 타이머 */
	FTimerHandle DecisionTimerHandle;

	/** 서버가 받은 결정 신호 구독 핸들 (GameCustom1 = 스텝, GameCustom2 = 구르기) */
	FDelegateHandle RemoteStepHandle;
	FDelegateHandle RemoteRollHandle;

	/** 이미 스텝/구르기가 시작됐는지 — 결정이 두 번 실행되는 것 방지 */
	bool bMoveStarted = false;

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

	/** 회피 종료 후 스태미나 회복을 지연시키는 GE. BP에서 GE_StaminaRegenDelay 지정 (Duration = SetByCaller Souls.SetByCaller.Duration) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Effect")
	TSubclassOf<UGameplayEffect> StaminaRegenDelayEffect;

	/** 회피 종료 후 스태미나 회복 지연 시간(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge", meta = (ClampMin = "0.0"))
	float StaminaRegenDelayDuration = 1.5f;

	// ── 26.11.13 추가 - 더블탭 구르기 설정 ────────────────────────
	/** 첫 입력 후 두 번째 입력을 기다리는 시간(초). 이 안에 다시 누르면 구르기, 지나면 스텝 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Roll", meta = (ClampMin = "0.0"))
	float DoubleTapWindow = 0.3f;

	/** [서버] 게스트의 결정 신호가 오지 않을 때 스텝으로 처리하기까지의 추가 여유 시간(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Roll", meta = (ClampMin = "0.0"))
	float RemoteDecisionTimeout = 0.5f;

	/** 구르기 몽타주 (한 방향 — 입력 방향으로 캐릭터를 돌린 뒤 재생) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Roll")
	TObjectPtr<UAnimMontage> RollMontage;

	/** 구르기 몽타주 시작 섹션. None이면 처음부터 재생 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Roll")
	FName RollSection = NAME_None;

	/** 구르기 확정 시 추가로 빼는 스태미나 GE (Instant, Scalable Float 감소). 부족하면 스텝으로 대체 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Roll")
	TSubclassOf<UGameplayEffect> RollExtraCostEffect;

	/** [서버 타이머] 구르기 몽타주 시작 후 i-frame 시작 시각(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Roll", meta = (ClampMin = "0.0"))
	float RollIFrameStartTime = 0.1f;

	/** [서버 타이머] 구르기 몽타주 시작 후 i-frame 종료 시각(초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Dodge|Roll", meta = (ClampMin = "0.0"))
	float RollIFrameEndTime = 0.6f;

};
