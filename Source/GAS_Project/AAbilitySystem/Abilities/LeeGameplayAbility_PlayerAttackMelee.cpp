// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbility_PlayerAttackMelee.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Animation/AnimMontage.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACharacter/LeeTargetLockComponent.h"

ULeeGameplayAbility_PlayerAttackMelee::ULeeGameplayAbility_PlayerAttackMelee(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 좌클릭(또는 지정 입력) 트리거 시 활성화 — 부모의 ActivationGroup(Exclusive_Blocking),
	// NetExecutionPolicy(ServerInitiated)는 그대로 상속해 사용한다.
	ActivationPolicy = ELeeAbilityActivationPolicy::OnInputTriggered;

	// 콤보 윈도우 이벤트 태그 기본값
	ComboWindowOpenTag = MyTags::Souls::Event_Attack_ComboWindowOpen;
	ComboWindowCloseTag = MyTags::Souls::Event_Attack_ComboWindowClose;
	ComboTransitionTag = MyTags::Souls::Event_Attack_ComboTransition;
}

// 콤보 첫 타 선택 — 부모의 랜덤 선택 대신 항상 AttackDataList[0]부터 시작
const FLeeMeleeAttackData* ULeeGameplayAbility_PlayerAttackMelee::SelectAttackData()
{
	CurrentComboIndex = 0;
	return AttackDataList.IsValidIndex(0) ? &AttackDataList[0] : nullptr;
}

// 콤보 윈도우/입력 Task를 먼저 등록한 뒤, 첫 타 몽타주를 재생한다
void ULeeGameplayAbility_PlayerAttackMelee::PlayAttackMontage(UAnimMontage* Montage, FName StartSection)
{
	ResetComboState();

	// 시작 프레임 이벤트 유실을 막기 위해 몽타주 ReadyForActivation보다 먼저 등록
	RegisterComboTasks();
	ListenForNextComboInput();

	// 첫 타는 처음부터 재생 (섹션 사용 안 함)
	// Super::PlayAttackMontage(부모 구현)가 재생 직전 UpdateAttackWarpTarget()을 호출한다
	Super::PlayAttackMontage(Montage, NAME_None);
}

// [모션워핑] 워프가 바라볼 대상 — 락온 중일 때만 락온 대상을 반환, 아니면 nullptr(부모가 제자리 공격으로 폴백)
AActor* ULeeGameplayAbility_PlayerAttackMelee::GetWarpFacingTarget() const
{
	const ULeeTargetLockComponent* Lock = ULeeTargetLockComponent::FindTargetLockComponent(GetAvatarActorFromActorInfo());
	return (Lock && Lock->IsLocked()) ? Lock->GetLockedTarget() : nullptr;
}

// 콤보 상태 초기화
void ULeeGameplayAbility_PlayerAttackMelee::ResetComboState()
{
	bComboWindowOpen = false;
	bFirstWindowOpened = false;
	bComboInputQueued = false;
	bBufferedInput = false;
	bTransitioningCombo = false;
	CurrentComboIndex = 0;
}

// ComboWindowOpen/Close/Transition 이벤트 대기 Task 등록 (어빌리티당 1회, 각 타 몽타주가 같은 태그로 계속 발사)
void ULeeGameplayAbility_PlayerAttackMelee::RegisterComboTasks()
{
	if (ComboWindowOpenTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* OpenTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, ComboWindowOpenTag, /*OptionalExternalOwner*/nullptr,
				/*OnlyTriggerOnce*/false, /*OnlyMatchExact*/true);

		OpenTask->EventReceived.AddDynamic(this, &ThisClass::OnComboWindowOpen);
		OpenTask->ReadyForActivation();
	}

	if (ComboWindowCloseTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* CloseTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, ComboWindowCloseTag, /*OptionalExternalOwner*/nullptr,
				/*OnlyTriggerOnce*/false, /*OnlyMatchExact*/true);

		CloseTask->EventReceived.AddDynamic(this, &ThisClass::OnComboWindowClose);
		CloseTask->ReadyForActivation();
	}

	if (ComboTransitionTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* TransitionTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, ComboTransitionTag, /*OptionalExternalOwner*/nullptr,
				/*OnlyTriggerOnce*/false, /*OnlyMatchExact*/true);

		TransitionTask->EventReceived.AddDynamic(this, &ThisClass::OnComboTransition);
		TransitionTask->ReadyForActivation();
	}
}

// WaitInputPress 등록 — 입력 수신 시 즉시 재등록해 다음 입력도 계속 받는다
void ULeeGameplayAbility_PlayerAttackMelee::ListenForNextComboInput()
{
	// 콤보 상태는 서버만 소유 — 클라이언트에서는 입력 리스너를 걸지 않는다 (ServerInitiated 규칙)
	if (!GetActorInfo().IsNetAuthority())
	{
		return;
	}

	UAbilityTask_WaitInputPress* InputTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
	InputTask->OnPress.AddDynamic(this, &ThisClass::OnComboInputPressed);
	InputTask->ReadyForActivation();
}

// 콤보 입력 허용 구간 시작 — 서버 전용. 선입력(버퍼)이 있으면 이 순간 예약으로 승격한다
void ULeeGameplayAbility_PlayerAttackMelee::OnComboWindowOpen(FGameplayEventData Payload)
{
	if (!GetActorInfo().IsNetAuthority() || !IsActive())
	{
		return;
	}

	bComboWindowOpen = true;
	bFirstWindowOpened = true;   // 이 시점 이후부터 선입력(윈도우 직전 입력) 버퍼링을 허용

	// 윈도우가 열리기 직전에 눌러둔 선입력이 있으면 예약으로 승격 (전환 노티파이에서 소비)
	if (bBufferedInput)
	{
		bBufferedInput = false;
		bComboInputQueued = true;
	}
}

// 콤보 입력 허용 구간 종료 — 서버 전용. 창만 닫고 예약(큐)은 유지한다
void ULeeGameplayAbility_PlayerAttackMelee::OnComboWindowClose(FGameplayEventData Payload)
{
	if (!GetActorInfo().IsNetAuthority() || !IsActive())
	{
		return;
	}

	bComboWindowOpen = false;
}

// 콤보 입력 수신 — 윈도우 중이면 예약만(즉시 전환 X), 닫혀 있으면 선입력으로 버퍼링
void ULeeGameplayAbility_PlayerAttackMelee::OnComboInputPressed(float TimeWaited)
{
	// 다음 입력도 계속 받을 수 있도록 즉시 재등록
	ListenForNextComboInput();

	if (!GetActorInfo().IsNetAuthority() || !IsActive())
	{
		return;
	}

	if (bComboWindowOpen)
	{
		// 실제 전환은 전환 노티파이(OnComboTransition)에서 실행 — 여기서는 예약만
		bComboInputQueued = true;
	}
	else if (bFirstWindowOpened)
	{
		// 첫 윈도우가 한 번 열린 뒤에만 선입력 허용 → OnComboWindowOpen에서 예약으로 승격
		bBufferedInput = true;
	}
	// else: 첫 콤보 윈도우가 열리기 전 입력 = 어빌리티를 활성화시킨 그 클릭 → 무시(콤보로 세지 않음)
}

// 전환 노티파이 수신 — 실제 콤보 전환이 실행되는 시점 (서버 전용)
void ULeeGameplayAbility_PlayerAttackMelee::OnComboTransition(FGameplayEventData Payload)
{
	if (!GetActorInfo().IsNetAuthority() || !IsActive())
	{
		return;
	}

	AdvanceComboIfQueued();
}

// 큐에 예약된 입력이 있으면 다음 타로 전환 — 전환 노티파이에서만 호출
void ULeeGameplayAbility_PlayerAttackMelee::AdvanceComboIfQueued()
{
	if (!bComboInputQueued)
	{
		return;
	}

	const int32 NextIndex = CurrentComboIndex + 1;
	if (!AttackDataList.IsValidIndex(NextIndex))
	{
		// 마지막 타 — 예약 무시 (콤보 끝)
		return;
	}

	bComboInputQueued = false;
	PlayComboAttack(NextIndex);
}

// 지정 인덱스의 타 몽타주로 전환 재생 — ComboEntry 섹션부터 시작해 준비동작을 생략한다
void ULeeGameplayAbility_PlayerAttackMelee::PlayComboAttack(int32 Index)
{
	if (!AttackDataList.IsValidIndex(Index) || !AttackDataList[Index].Montage)
	{
		return;
	}

	// 현재 타 데이터 교체 — 데미지/트레이스/스태미나 비용이 이 타 기준으로 갱신된다
	CurrentAttackData = AttackDataList[Index];
	CurrentComboIndex = Index;

	// 새 타로 넘어가므로 윈도우/선입력/예약 상태를 리셋 (다음 타의 윈도우 Notify를 새로 기다린다)
	bComboWindowOpen = false;
	bComboInputQueued = false;
	bBufferedInput = false;

	// 이전 몽타주가 중단되며 OnMontageInterrupted가 불리지만, 콤보 전환이므로 어빌리티를 끝내면 안 된다
	bTransitioningCombo = true;

	// 준비동작을 생략하고 ComboEntry 섹션부터 재생 (없으면 안전 폴백으로 처음부터)
	// (Super::PlayAttackMontage가 CurrentAttackData 갱신 후 워프 타깃도 다시 갱신한다)
	const FName EntrySection = ResolveComboEntrySection(CurrentAttackData.Montage);

	// 부모 재생 로직 재사용 — 새 PlayMontageAndWait Task를 만들어 다음 타 몽타주로 블렌드
	Super::PlayAttackMontage(CurrentAttackData.Montage, EntrySection);
}

// 다음 몽타주에 ComboEntry 섹션이 있는지 확인 — 없으면 경고 로그 후 처음부터 재생하도록 폴백
FName ULeeGameplayAbility_PlayerAttackMelee::ResolveComboEntrySection(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return NAME_None;
	}

	if (Montage->GetSectionIndex(ComboEntrySectionName) == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerAttackMelee] %s에 [%s] 섹션 없음 → 처음부터 재생"),
			*Montage->GetName(), *ComboEntrySectionName.ToString());
		return NAME_None;
	}

	return ComboEntrySectionName;
}

// 마지막 타 완주 / 블렌드아웃 → 콤보 종료
void ULeeGameplayAbility_PlayerAttackMelee::OnMontageCompleted()
{
	ResetComboState();
	Super::OnMontageCompleted();
}

// 몽타주 중단 — 콤보 전환에 의한 중단이면 어빌리티를 유지, 진짜 외부 취소(피격 등)면 종료
void ULeeGameplayAbility_PlayerAttackMelee::OnMontageInterrupted()
{
	if (bTransitioningCombo)
	{
		// 다음 타 재생을 위해 이전 몽타주 Task가 발사한 중단 콜백 — 흡수하고 어빌리티는 그대로 진행
		bTransitioningCombo = false;
		return;
	}

	ResetComboState();
	Super::OnMontageInterrupted();
}

// 스태미나 차감 — 각 타 데이터의 [0] 비용을 쓰고, 콤보 인덱스를 중복키로 넘긴다
void ULeeGameplayAbility_PlayerAttackMelee::OnAttackStepEventReceived(FGameplayEventData Payload)
{
	if (!GetActorInfo().IsNetAuthority() || !IsActive())
	{
		return;
	}

	// CostIndex=0 : 개별 몽타주라 각 타 데이터의 첫 비용이 그 타의 비용
	// DedupKey=CurrentComboIndex : 타마다 다른 키라 같은 타 안에서만 중복 차감을 막는다
	TryCommitAttackStepCost(/*CostIndex*/0, /*DedupKey*/CurrentComboIndex);
}
