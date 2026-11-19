// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility_AttackMelee.h"
#include "LeeGameplayAbility_PlayerAttackMelee.generated.h"

class UInputAction;

/**
 * 26.11.16 추가 - 차지(홀드) 공격 1단계 설정.
 * 차지 구간에서 버튼을 HoldTime 이상 누르고 있으면 이 단계가 되고, 떼면 이 단계의 값으로 벤다.
 */
USTRUCT(BlueprintType)
struct FLeeChargeLevelData
{
	GENERATED_BODY()

	/** 이 단계에 도달하는 홀드 시간(초). 차지 시작(몽타주가 멈춘 순간)부터 잰다. 단계 순서대로 커져야 한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float HoldTime = 0.4f;

	/** 데미지 배율 — 그 타의 BaseDamage에 곱한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.5f;

	/** 떼는 순간 추가로 소모하는 스태미나. 스태미나가 이 값보다 적으면 이 단계로 올라가지 않는다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float ExtraStaminaCost = 10.0f;

	/** 켜면 이 단계의 넉백으로 바꾼다 (끄면 그 타의 넉백 그대로) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (InlineEditConditionToggle))
	bool bOverrideKnockback = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bOverrideKnockback"))
	FLeeKnockbackParams Knockback;

	/** 켜면 이 단계의 히트스톱으로 바꾼다 (끄면 그 타의 히트스톱 그대로) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (InlineEditConditionToggle))
	bool bOverrideHitStop = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bOverrideHitStop"))
	FLeeHitStopParams HitStop;

	/** 이 단계에서 추가로 실을 공격 속성 태그 (예: Souls.DamageType.Attack.Heavy → 적 강경직) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer ExtraDamageTypeTags;

	/** 이 단계에 도달한 순간 실행할 GameplayCue (카메라 흔들림·번쩍임·사운드). RawMagnitude = 단계 번호(1부터) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "GameplayCue"))
	FGameplayTag ReachedCueTag;

	/** 떼었을 때 이동할 몽타주 섹션. 비우거나 섹션이 없으면 멈춘 곳부터 이어서 재생 (단계별로 다른 베기를 넣을 때 사용) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName ReleaseSectionName;
};

/**
 * Player 전용 근접 콤보 공격 어빌리티 (개별 몽타주 방식).
 * ULeeGameplayAbility_AttackMelee(Enemy 베이스)를 상속하여 트레이스/스태미나/데미지 처리는 그대로 재사용하고,
 * 좌클릭 입력 트리거 + "타마다 별도 몽타주"를 이어 붙이는 콤보 로직만 추가한다.
 *
 * 콤보 구조 (섹션 방식 X → 개별 몽타주 방식 O):
 *  - AttackDataList = 순차 콤보 배열. [0]=1타, [1]=2타, [2]=3타, [3]=4타 ...
 *    각 엔트리는 자기 타의 몽타주 + 데미지 + StaminaCostPerStep[0](그 타의 비용)을 담는다.
 *  - 각 타 몽타주에 배치할 Notify:
 *     1) Lee Gameplay Event Window (Server) — Begin=ComboWindowOpenTag, End=ComboWindowCloseTag
 *        → 이 구간이 "다음 타 입력 허용 구간"
 *     2) Lee Gameplay Event (Server) — EventTag=Souls.Events.Attack.CommitStep (스태미나 비용 1회)
 *     3) UANS_WeaponTrace — 무기 소켓 트레이스 (히트 판정)
 *
 * 콤보 진행 규약 (서버 권위 — ServerInitiated) — 전환 노티파이 방식:
 *  - ComboWindowOpen~ComboWindowClose 구간에서 공격 입력이 들어오면 즉시 전환하지 않고 "큐에 예약"만 한다.
 *  - 실제 전환은 각 타 몽타주의 전환 노티파이(Souls.Events.Attack.ComboTransition) 프레임에서 실행된다.
 *  - 윈도우가 열리기 직전에 누른 입력은 버퍼링했다가 윈도우가 열리는 순간 큐로 승격한다(입력 선입력 허용).
 *  - 전환 시 다음 타 몽타주는 처음부터가 아니라 ComboEntrySectionName 섹션부터 재생해 준비동작을 생략한다(첫 타 제외).
 *  - 한 번의 예약은 전환 시 1회만 소비된다. 마지막 타에서의 추가 입력은 무시한다.
 *  - 콤보 상태는 서버 인스턴스만 갱신하며, 클라이언트는 서버가 재생시킨 몽타주 리플리케이션만 재생한다.
 *
 * 26.11.16 추가 - 차지(홀드) 공격 (ChargeLevels가 비어 있으면 동작하지 않음 → 약공격은 영향 없음):
 *  - 각 타 몽타주에 Lee Gameplay Event 노티파이(EventTag=Souls.Events.Attack.ChargeStart, bSendOnLocalClient=true)를 두면
 *    그 지점에서 버튼을 누르고 있을 때 몽타주가 멈추고 차지가 시작된다. 노티파이가 없는 타는 차지 없음.
 *  - 누른 시간에 따라 ChargeLevels 단계가 오르고(단계마다 Cue 실행), 떼거나 최대 시간이 지나면 그 단계 값으로 이어서 벤다.
 *  - 게스트는 자기 화면 기준으로 단계를 정해 서버에 알리고(GameCustom2 + 단계 값), 서버는 그 신호가 올 때까지 같은 지점에서 멈춰 기다린다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility_PlayerAttackMelee : public ULeeGameplayAbility_AttackMelee
{
	GENERATED_BODY()

public:
	ULeeGameplayAbility_PlayerAttackMelee(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/** 콤보 첫 타(AttackDataList[0])를 반환 — 부모의 랜덤 선택 대신 순차 콤보 시작점으로 사용 */
	virtual const FLeeMeleeAttackData* SelectAttackData() override;

	/** 콤보 윈도우/입력 Task를 등록한 뒤 첫 타 몽타주를 재생 */
	virtual void PlayAttackMontage(UAnimMontage* Montage, FName StartSection) override;

	virtual void OnMontageCompleted() override;
	virtual void OnMontageInterrupted() override;

	/** 26.11.04 추가 - 서버의 원격 콤보 전환 신호 구독 해제 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** [모션워핑] 워프가 바라볼 대상 — 락온 대상(있고 락온 중일 때만)을 반환 */
	virtual AActor* GetWarpFacingTarget() const override;

	/** [모션워핑] 락온 대상이 없을 때 — 현재 이동 입력(가속도) 방향을 반환. 입력이 없으면 false */
	virtual bool GetWarpFallbackDirection(FVector& OutDirection) const override;

	/** 각 타 데이터의 [0] 비용을 쓰되 콤보 인덱스를 중복키로 넘겨 스태미나를 차감한다 */
	virtual void OnAttackStepEventReceived(FGameplayEventData Payload) override;

	/** 26.11.17 추가 - Player 콤보는 타마다 별도 몽타주라 첫 타 비용만 확인 (기존 동작 유지) */
	virtual float GetStaminaRequiredToStart(const FLeeMeleeAttackData& AttackData) const override;

	/** 26.11.17 추가 - Player는 도중에 스태미나가 모자라면 기존처럼 공격을 끊는다 */
	virtual bool ShouldEndOnStaminaShortage() const override { return true; }

	/** 콤보 입력 허용 구간 시작 이벤트 태그. 기본값: Souls.Events.Attack.ComboWindowOpen */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Combo")
	FGameplayTag ComboWindowOpenTag;

	/** 콤보 입력 허용 구간 종료 이벤트 태그. 기본값: Souls.Events.Attack.ComboWindowClose */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Combo")
	FGameplayTag ComboWindowCloseTag;

	/** 콤보 전환 실행 시점 이벤트 태그. 기본값: Souls.Events.Attack.ComboTransition */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Combo")
	FGameplayTag ComboTransitionTag;

	/** 다음 타 몽타주가 재생을 시작할 섹션 이름(준비동작 생략 진입점). 기본값: "ComboEntry" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Combo")
	FName ComboEntrySectionName = TEXT("ComboEntry");

	/** 26.11.16 추가 - 차지 시작 이벤트 태그. 기본값: Souls.Events.Attack.ChargeStart */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Charge")
	FGameplayTag ChargeStartTag;

	/** 26.11.16 추가 - 누르고 있는지 확인할 입력 액션 (예: IA_HeavyAttack_souls). 비우면 차지하지 않는다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Charge")
	TObjectPtr<const UInputAction> ChargeInputAction;

	/** 26.11.16 추가 - 차지 단계 목록 ([0]=1단계, [1]=2단계 ...). 비어 있으면 차지 기능을 쓰지 않는다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Charge")
	TArray<FLeeChargeLevelData> ChargeLevels;

	/** 26.11.16 추가 - 마지막 단계 시간 이후 이만큼(초) 더 누르고 있으면 자동으로 벤다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Charge", meta = (ClampMin = "0.0"))
	float AutoReleaseDelay = 0.3f;

	/** 26.11.16 추가 - 차지 시작(몽타주가 멈춘 순간)에 실행할 Cue. RawMagnitude = 0 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Charge", meta = (Categories = "GameplayCue"))
	FGameplayTag ChargeStartCueTag;

	/** 26.11.16 추가 - 차지를 떼고 베기 시작할 때 실행할 Cue (흔들림 정지 등). RawMagnitude = 도달 단계 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Attack|Charge", meta = (Categories = "GameplayCue"))
	FGameplayTag ChargeReleaseCueTag;

private:
	/** ComboWindowOpen/Close/Transition WaitGameplayEvent Task 등록 (어빌리티당 1회 — 각 타 몽타주 Notify가 같은 태그로 계속 쏨) */
	void RegisterComboTasks();

	/** WaitInputPress 등록/재등록 — 입력을 받을 때마다 즉시 다시 걸어 다음 입력도 계속 수신 */
	void ListenForNextComboInput();

	/** 콤보 상태 초기화 (재생 시작/완료/중단 시 호출) */
	void ResetComboState();

	/** 큐에 예약된 입력이 있으면 다음 타로 전환 — 전환 노티파이 수신 시에만 호출 */
	void AdvanceComboIfQueued();

	/** 지정 인덱스의 타 몽타주로 전환 재생 (이전 몽타주는 중단되지만 bTransitioningCombo로 어빌리티는 유지) */
	void PlayComboAttack(int32 Index);

	/** 다음 몽타주의 ComboEntry 섹션 존재 확인 — 없으면 경고 로그 후 NAME_None(처음부터)으로 안전 폴백 */
	FName ResolveComboEntrySection(const UAnimMontage* Montage) const;

	UFUNCTION()
	void OnComboWindowOpen(FGameplayEventData Payload);

	UFUNCTION()
	void OnComboWindowClose(FGameplayEventData Payload);

	UFUNCTION()
	void OnComboTransition(FGameplayEventData Payload);

	UFUNCTION()
	void OnComboInputPressed(float TimeWaited);

	/**
	 * 26.11.04 추가 - 서버에서 도는 "게스트 캐릭터"의 인스턴스인지 (서버 권한 + 로컬 조종 아님).
	 * 이 인스턴스는 자기 노티파이로 콤보를 진행하지 않고, 게스트가 보낸 전환 신호로만 다음 타를 재생한다.
	 */
	bool IsServerForRemotePlayer() const;

	/** 26.11.04 추가 - [게스트] 다음 타로 넘어갔음을 서버에 알린다 (GAS 복제 이벤트 GameCustom1) */
	void SendComboAdvanceToServer();

	/** 26.11.04 추가 - [서버, 원격 플레이어] 게스트의 전환 신호 구독 시작 */
	void ListenForRemoteComboAdvance();

	/** 26.11.04 추가 - [서버, 원격 플레이어] 전환 신호 수신 → 다음 타 재생 */
	void OnRemoteComboAdvance();

	/** 26.11.04 추가 - 원격 콤보 전환 신호 델리게이트 핸들 (EndAbility에서 해제) */
	FDelegateHandle RemoteComboAdvanceHandle;

	/** 콤보 입력 허용 여부 (서버 전용 상태) */
	bool bComboWindowOpen = false;

	/**
	 * 이번 활성화에서 첫 콤보 윈도우가 한 번이라도 열렸는지.
	 * 첫 윈도우가 열리기 전 입력은 "어빌리티를 활성화시킨 그 클릭"뿐이므로 무시한다
	 * (WaitInputPress가 활성화 입력을 콤보 입력으로 오인해 좌클릭 1번에 2타가 나가는 문제 차단).
	 */
	bool bFirstWindowOpened = false;

	/** 윈도우 중 입력이 들어와 다음 타 전환이 예약되었는지 — 전환 노티파이에서 소비 */
	bool bComboInputQueued = false;

	/** 윈도우가 열리기 직전에 눌린 선입력 저장 — 윈도우가 열리는 순간 예약으로 승격 */
	bool bBufferedInput = false;

	/** 콤보 전환으로 인한 이전 몽타주 중단인지 — true면 OnMontageInterrupted가 어빌리티를 끝내지 않는다 */
	bool bTransitioningCombo = false;

	/** 현재 재생 중인 콤보 타 인덱스 (AttackDataList 기준) */
	int32 CurrentComboIndex = 0;

	// ===== 26.11.16 추가 - 차지(홀드) 공격 =====

	/** 차지 시작 이벤트 수신 — 누르고 있으면(게스트의 서버 인스턴스는 항상) 몽타주를 멈추고 차지를 시작한다 */
	UFUNCTION()
	void OnChargeStart(FGameplayEventData Payload);

	/** 몽타주를 멈추고 단계 확인 타이머를 시작한다 */
	void BeginCharge();

	/** 0.02초마다 호출 — 단계 올리기, 뗌/자동 발사 판정 */
	void TickCharge();

	/** 차지를 끝내고 지정 단계 값으로 이어서 벤다 */
	void ReleaseCharge(int32 Level);

	/** 단계 값(데미지 배율·넉백·히트스톱·속성 태그)을 이번 타 데이터에 반영하고, 서버면 추가 스태미나를 차감한다 */
	void ApplyChargeLevelToAttack(int32 Level);

	/** 차지 타이머·상태 정리 (어빌리티 종료·다음 타 전환 시) */
	void ResetChargeState();

	/** [로컬] ChargeInputAction에 매핑된 키를 지금 누르고 있는지 */
	bool IsChargeInputHeld() const;

	/** 현재 스태미나로 해당 단계(1부터)의 추가 비용을 낼 수 있는지 */
	bool CanAffordChargeLevel(int32 Level) const;

	/** 마지막 단계 시간 + AutoReleaseDelay — 이 시간이 지나면 자동 발사 */
	float GetChargeAutoReleaseTime() const;

	/** 현재 공격 몽타주 재생 속도 변경 (서버는 다른 클라이언트에 복제, 게스트는 자기 화면만) */
	void SetAttackMontagePlayRate(float PlayRate);

	/** 현재 공격 몽타주의 섹션으로 이동 (서버는 복제, 게스트는 자기 화면만) */
	void JumpAttackMontageToSection(FName SectionName);

	/** 차지 연출 Cue 실행 — 직접 조종하는 쪽은 바로, 서버는 다른 클라이언트에게 보낸다 */
	void ExecuteChargeCue(const FGameplayTag& CueTag, int32 Level);

	/** [게스트] 이번 차지 결과 단계를 서버에 알린다 (GameCustom2 + 단계 값) */
	void SendChargeReleaseToServer(int32 Level);

	/** [서버, 원격 플레이어] 게스트의 차지 결과 신호 구독 (이미 도착해 있으면 바로 처리) */
	void ListenForRemoteChargeRelease();

	/** [서버, 원격 플레이어] 게스트 신호 수신 → 그 단계로 이어서 벤다 (아직 차지 지점 전이면 신호를 남겨 둔다) */
	void OnRemoteChargeRelease();

	/** 원격 차지 신호 델리게이트 핸들 (EndAbility에서 해제) */
	FDelegateHandle RemoteChargeReleaseHandle;

	/** 차지 단계 확인 타이머 */
	FTimerHandle ChargeTimerHandle;

	/** 지금 차지 중(몽타주 멈춤)인지 */
	bool bCharging = false;

	/** 차지를 시작한 월드 시간 */
	float ChargeStartTime = 0.0f;

	/** 현재 도달한 차지 단계 (0 = 차지 없음, 1부터 ChargeLevels 인덱스+1) */
	int32 CurrentChargeLevel = 0;
};
