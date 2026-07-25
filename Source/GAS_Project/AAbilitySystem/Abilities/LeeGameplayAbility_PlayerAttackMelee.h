// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LeeGameplayAbility_AttackMelee.h"
#include "LeeGameplayAbility_PlayerAttackMelee.generated.h"

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

	/** [모션워핑] 워프가 바라볼 대상 — 락온 대상(있고 락온 중일 때만)을 반환 */
	virtual AActor* GetWarpFacingTarget() const override;

	/** 각 타 데이터의 [0] 비용을 쓰되 콤보 인덱스를 중복키로 넘겨 스태미나를 차감한다 */
	virtual void OnAttackStepEventReceived(FGameplayEventData Payload) override;

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
};
