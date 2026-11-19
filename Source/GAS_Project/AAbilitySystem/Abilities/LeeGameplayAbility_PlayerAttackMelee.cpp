// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbility_PlayerAttackMelee.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACharacter/LeeTargetLockComponent.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"
#include "Animation/AnimInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "TimerManager.h"

// 26.11.04 추가 - 콤보 진단 로그용: 이 인스턴스가 서버/호스트 본인/클라이언트 중 어디서 도는지 문자열로 반환
static const TCHAR* GetComboNetRole(const UGameplayAbility* Ability)
{
	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		return TEXT("Client");
	}
	return ActorInfo->IsLocallyControlled() ? TEXT("Server(Host)") : TEXT("Server(Remote)");
}

ULeeGameplayAbility_PlayerAttackMelee::ULeeGameplayAbility_PlayerAttackMelee(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 좌클릭(또는 지정 입력) 트리거 시 활성화 — 부모의 ActivationGroup(Exclusive_Blocking),
	// NetExecutionPolicy(LocalPredicted)는 그대로 상속해 사용한다.
	// 26.11.02 - 콤보 판정은 서버와 로컬 클라이언트가 각자 자기 몽타주 기준으로 수행한다
	// (로컬 클라이언트는 서버가 복제한 몽타주를 무시하므로 콤보 전환을 직접 재생해야 한다).
	ActivationPolicy = ELeeAbilityActivationPolicy::OnInputTriggered;

	// 콤보 윈도우 이벤트 태그 기본값
	ComboWindowOpenTag = MyTags::Souls::Event_Attack_ComboWindowOpen;
	ComboWindowCloseTag = MyTags::Souls::Event_Attack_ComboWindowClose;
	ComboTransitionTag = MyTags::Souls::Event_Attack_ComboTransition;

	// 26.11.16 추가 - 차지 시작 이벤트 태그 기본값
	ChargeStartTag = MyTags::Souls::Event_Attack_ChargeStart;
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
	UE_LOG(LogLee, Log, TEXT("[Combo] %s 1타 시작"), GetComboNetRole(this));

	// 시작 프레임 이벤트 유실을 막기 위해 몽타주 ReadyForActivation보다 먼저 등록
	RegisterComboTasks();
	ListenForNextComboInput();

	// 26.11.04 추가 - 서버의 게스트 캐릭터는 게스트가 보내는 전환 신호로 콤보를 진행한다
	if (IsServerForRemotePlayer())
	{
		ListenForRemoteComboAdvance();
	}

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

// [모션워핑] 락온이 아닐 때 공격 방향 = 현재 이동 입력 방향
// 서버에서도 원격 클라이언트의 입력을 알 수 있도록 GetLastMovementInputVector 대신 가속도(ServerMove로 전달됨)를 사용
bool ULeeGameplayAbility_PlayerAttackMelee::GetWarpFallbackDirection(FVector& OutDirection) const
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const UCharacterMovementComponent* MoveComp = Character ? Character->GetCharacterMovement() : nullptr;
	if (!MoveComp)
	{
		return false;
	}

	OutDirection = MoveComp->GetCurrentAcceleration().GetSafeNormal2D();
	return !OutDirection.IsNearlyZero();
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

	// 26.11.16 추가 - 차지 단계가 설정된 어빌리티만 차지 시작 이벤트를 받는다 (약공격은 ChargeLevels가 비어 있어 등록 안 함)
	if (ChargeStartTag.IsValid() && !ChargeLevels.IsEmpty())
	{
		UAbilityTask_WaitGameplayEvent* ChargeTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, ChargeStartTag, /*OptionalExternalOwner*/nullptr,
				/*OnlyTriggerOnce*/false, /*OnlyMatchExact*/true);

		ChargeTask->EventReceived.AddDynamic(this, &ThisClass::OnChargeStart);
		ChargeTask->ReadyForActivation();
	}
}

// WaitInputPress 등록 — 입력 수신 시 즉시 재등록해 다음 입력도 계속 받는다
void ULeeGameplayAbility_PlayerAttackMelee::ListenForNextComboInput()
{
	// 26.11.02 수정 - 서버(복제된 입력)와 로컬 클라이언트(직접 입력) 양쪽에서 리스너를 건다
	UAbilityTask_WaitInputPress* InputTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
	InputTask->OnPress.AddDynamic(this, &ThisClass::OnComboInputPressed);
	InputTask->ReadyForActivation();
}

// 콤보 입력 허용 구간 시작 — 서버·로컬 클라이언트 각자. 선입력(버퍼)이 있으면 이 순간 예약으로 승격한다
void ULeeGameplayAbility_PlayerAttackMelee::OnComboWindowOpen(FGameplayEventData Payload)
{
	if (!IsActive())
	{
		return;
	}

	bComboWindowOpen = true;
	bFirstWindowOpened = true;
	UE_LOG(LogLee, Log, TEXT("[Combo] %s 윈도우 열림 Index=%d Buffered=%d"), GetComboNetRole(this), CurrentComboIndex, bBufferedInput);   // 이 시점 이후부터 선입력(윈도우 직전 입력) 버퍼링을 허용

	// 윈도우가 열리기 직전에 눌러둔 선입력이 있으면 예약으로 승격 (전환 노티파이에서 소비)
	if (bBufferedInput)
	{
		bBufferedInput = false;
		bComboInputQueued = true;
	}
}

// 콤보 입력 허용 구간 종료 — 서버·로컬 클라이언트 각자. 창만 닫고 예약(큐)은 유지한다
void ULeeGameplayAbility_PlayerAttackMelee::OnComboWindowClose(FGameplayEventData Payload)
{
	if (!IsActive())
	{
		return;
	}

	bComboWindowOpen = false;
	UE_LOG(LogLee, Log, TEXT("[Combo] %s 윈도우 닫힘 Index=%d Queued=%d"), GetComboNetRole(this), CurrentComboIndex, bComboInputQueued);
}

// 콤보 입력 수신 — 윈도우 중이면 예약만(즉시 전환 X), 닫혀 있으면 선입력으로 버퍼링
void ULeeGameplayAbility_PlayerAttackMelee::OnComboInputPressed(float TimeWaited)
{
	// 다음 입력도 계속 받을 수 있도록 즉시 재등록
	ListenForNextComboInput();

	if (!IsActive())
	{
		return;
	}

	UE_LOG(LogLee, Log, TEXT("[Combo] %s 입력 수신 Index=%d WindowOpen=%d FirstWindowOpened=%d"),
		GetComboNetRole(this), CurrentComboIndex, bComboWindowOpen, bFirstWindowOpened);

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

// 전환 노티파이 수신 — 실제 콤보 전환이 실행되는 시점 (서버·로컬 클라이언트 각자)
void ULeeGameplayAbility_PlayerAttackMelee::OnComboTransition(FGameplayEventData Payload)
{
	if (!IsActive())
	{
		return;
	}

	AdvanceComboIfQueued();
}

// 큐에 예약된 입력이 있으면 다음 타로 전환 — 전환 노티파이에서만 호출
void ULeeGameplayAbility_PlayerAttackMelee::AdvanceComboIfQueued()
{
	UE_LOG(LogLee, Log, TEXT("[Combo] %s 전환 노티파이 Index=%d Queued=%d"), GetComboNetRole(this), CurrentComboIndex, bComboInputQueued);

	if (!bComboInputQueued)
	{
		return;
	}

	// 26.11.04 추가 - 서버의 게스트 캐릭터는 여기서 전환하지 않는다 (서버 노티파이가 빠지면 게스트와 어긋나므로 게스트 신호만 따른다)
	if (IsServerForRemotePlayer())
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

	// 26.11.16 추가 - 이전 타의 차지 상태가 남지 않도록 정리
	ResetChargeState();

	// 현재 타 데이터 교체 — 데미지/트레이스/스태미나 비용이 이 타 기준으로 갱신된다 (이전 타의 차지 배율도 여기서 원래 값으로 돌아간다)
	CurrentAttackData = AttackDataList[Index];
	CurrentComboIndex = Index;
	UE_LOG(LogLee, Log, TEXT("[Combo] %s 다음 타 재생 Index=%d"), GetComboNetRole(this), Index);

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

	// 26.11.04 추가 - 게스트가 다음 타로 넘어갔으면 서버도 같은 타를 재생하도록 알린다
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (ActorInfo && !ActorInfo->IsNetAuthority() && ActorInfo->IsLocallyControlled())
	{
		SendComboAdvanceToServer();
	}
}

// 26.11.04 추가 - 서버 권한이면서 로컬 조종이 아닌 인스턴스 = 서버에서 도는 게스트 캐릭터
bool ULeeGameplayAbility_PlayerAttackMelee::IsServerForRemotePlayer() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	return ActorInfo && ActorInfo->IsNetAuthority() && !ActorInfo->IsLocallyControlled();
}

// 26.11.04 추가 - [게스트] "한 타 전진" 신호를 서버로 보낸다. 신뢰성 RPC라 순서대로 도착하므로 횟수 = 전진할 타 수
void ULeeGameplayAbility_PlayerAttackMelee::SendComboAdvanceToServer()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FScopedPredictionWindow ScopedPrediction(ASC, true);
	ASC->ServerSetReplicatedEvent(EAbilityGenericReplicatedEvent::GameCustom1, CurrentSpecHandle,
		GetCurrentActivationInfo().GetActivationPredictionKey(), ASC->ScopedPredictionKey);
	UE_LOG(LogLee, Log, TEXT("[Combo] %s 서버에 전환 신호 전송 Index=%d"), GetComboNetRole(this), CurrentComboIndex);
}

// 26.11.04 추가 - [서버, 원격 플레이어] 이번 활성화의 전환 신호를 구독하고, 이미 도착해 있으면 바로 처리
void ULeeGameplayAbility_PlayerAttackMelee::ListenForRemoteComboAdvance()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || RemoteComboAdvanceHandle.IsValid())
	{
		return;
	}

	const FPredictionKey ActivationKey = GetCurrentActivationInfo().GetActivationPredictionKey();
	RemoteComboAdvanceHandle = ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::GameCustom1, CurrentSpecHandle, ActivationKey)
		.AddUObject(this, &ThisClass::OnRemoteComboAdvance);
	ASC->CallReplicatedEventDelegateIfSet(EAbilityGenericReplicatedEvent::GameCustom1, CurrentSpecHandle, ActivationKey);
}

// 26.11.04 추가 - [서버, 원격 플레이어] 신호를 소비하고 다음 타를 재생 (데미지·스태미나 판정은 계속 서버 몽타주 기준)
void ULeeGameplayAbility_PlayerAttackMelee::OnRemoteComboAdvance()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ConsumeGenericReplicatedEvent(EAbilityGenericReplicatedEvent::GameCustom1, CurrentSpecHandle,
			GetCurrentActivationInfo().GetActivationPredictionKey());
	}

	const int32 NextIndex = CurrentComboIndex + 1;
	UE_LOG(LogLee, Log, TEXT("[Combo] %s 게스트 전환 신호 수신 → Index=%d"), GetComboNetRole(this), NextIndex);

	if (IsActive() && AttackDataList.IsValidIndex(NextIndex))
	{
		PlayComboAttack(NextIndex);
	}
}

// 26.11.04 추가 - 원격 콤보 전환 신호 구독 해제 후 부모 종료 처리
void ULeeGameplayAbility_PlayerAttackMelee::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (RemoteComboAdvanceHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::GameCustom1, Handle, ActivationInfo.GetActivationPredictionKey())
				.Remove(RemoteComboAdvanceHandle);
		}
		RemoteComboAdvanceHandle.Reset();
	}

	// 26.11.16 추가 - 차지 타이머 정리 + 원격 차지 신호 구독 해제
	ResetChargeState();
	if (RemoteChargeReleaseHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::GameCustom2, Handle, ActivationInfo.GetActivationPredictionKey())
				.Remove(RemoteChargeReleaseHandle);
		}
		RemoteChargeReleaseHandle.Reset();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
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
	UE_LOG(LogLee, Log, TEXT("[Combo] %s 몽타주 완료 → 어빌리티 종료 Index=%d"), GetComboNetRole(this), CurrentComboIndex);
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

	UE_LOG(LogLee, Log, TEXT("[Combo] %s 몽타주 중단(외부) → 어빌리티 종료 Index=%d"), GetComboNetRole(this), CurrentComboIndex);
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

// 26.11.17 추가 - Player 콤보는 타마다 별도 몽타주 → 이 타의 첫 비용만 확인 (Enemy는 모든 타 합계)
float ULeeGameplayAbility_PlayerAttackMelee::GetStaminaRequiredToStart(const FLeeMeleeAttackData& AttackData) const
{
	return AttackData.StaminaCostPerStep.IsValidIndex(0) ? AttackData.StaminaCostPerStep[0] : 0.0f;
}

// ===================================================================
// 26.11.16 추가 - 차지(홀드) 공격
// ===================================================================

// 차지 시작 지점 도달 — 직접 조종하는 쪽은 누르고 있을 때만 차지, 서버의 게스트 캐릭터는 게스트 신호가 올 때까지 멈춰 기다린다
void ULeeGameplayAbility_PlayerAttackMelee::OnChargeStart(FGameplayEventData Payload)
{
	if (!IsActive() || bCharging || ChargeLevels.IsEmpty())
	{
		return;
	}

	const FGameplayAbilityActorInfo& Info = GetActorInfo();

	if (Info.IsLocallyControlled())
	{
		if (IsChargeInputHeld())
		{
			BeginCharge();
		}
		else if (!Info.IsNetAuthority())
		{
			// 게스트가 누르지 않음 → 일반 베기. 서버는 이 지점에서 멈춰 기다리므로 0단계로 바로 알려 준다
			SendChargeReleaseToServer(0);
		}
		return;
	}

	if (IsServerForRemotePlayer())
	{
		BeginCharge();
		ListenForRemoteChargeRelease();
	}
}

// 몽타주를 멈추고 단계 확인 타이머를 시작한다
void ULeeGameplayAbility_PlayerAttackMelee::BeginCharge()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bCharging = true;
	CurrentChargeLevel = 0;
	ChargeStartTime = World->GetTimeSeconds();

	SetAttackMontagePlayRate(0.0f);
	ExecuteChargeCue(ChargeStartCueTag, 0);
	World->GetTimerManager().SetTimer(ChargeTimerHandle, this, &ThisClass::TickCharge, 0.02f, /*bLoop*/true);

	UE_LOG(LogLee, Log, TEXT("[Charge] %s 차지 시작 Index=%d"), GetComboNetRole(this), CurrentComboIndex);
}

// 0.02초마다 — 시간이 되면 단계를 올리고(Cue 실행), 떼었거나 최대 시간이 지나면 벤다
void ULeeGameplayAbility_PlayerAttackMelee::TickCharge()
{
	UWorld* World = GetWorld();
	if (!IsActive() || !bCharging || !World)
	{
		ResetChargeState();
		return;
	}

	const float Elapsed = World->GetTimeSeconds() - ChargeStartTime;

	// 다음 단계 시간이 됐고 스태미나도 충분하면 한 단계 올린다
	const int32 NextLevel = CurrentChargeLevel + 1;
	if (ChargeLevels.IsValidIndex(NextLevel - 1)
		&& Elapsed >= ChargeLevels[NextLevel - 1].HoldTime
		&& CanAffordChargeLevel(NextLevel))
	{
		CurrentChargeLevel = NextLevel;
		ExecuteChargeCue(ChargeLevels[NextLevel - 1].ReachedCueTag, NextLevel);
		UE_LOG(LogLee, Log, TEXT("[Charge] %s %d단계 도달 (%.2f초)"), GetComboNetRole(this), NextLevel, Elapsed);
	}

	if (GetActorInfo().IsLocallyControlled())
	{
		// 직접 조종하는 쪽: 떼었거나 최대 시간이 지나면 지금 단계로 벤다
		if (!IsChargeInputHeld() || Elapsed >= GetChargeAutoReleaseTime())
		{
			ReleaseCharge(CurrentChargeLevel);
		}
	}
	else if (Elapsed >= GetChargeAutoReleaseTime() + 1.0f)
	{
		// 서버의 게스트 캐릭터: 신호가 끝내 안 오면(연결 지연 등) 멈춘 채로 남지 않도록 지금 단계로 벤다
		UE_LOG(LogLee, Warning, TEXT("[Charge] %s 게스트 차지 신호 없음 → 자동 발사 Level=%d"), GetComboNetRole(this), CurrentChargeLevel);
		ReleaseCharge(CurrentChargeLevel);
	}
}

// 차지 종료 — 단계 값 반영 → (섹션 이동) → 방향 다시 맞추기 → 이어서 재생
void ULeeGameplayAbility_PlayerAttackMelee::ReleaseCharge(int32 Level)
{
	if (!bCharging)
	{
		return;
	}
	ResetChargeState();

	// 범위를 벗어난 값과 스태미나가 모자란 단계는 낮춘다 (게스트가 보낸 값도 서버가 다시 확인)
	Level = FMath::Clamp(Level, 0, ChargeLevels.Num());
	while (Level > 0 && !CanAffordChargeLevel(Level))
	{
		--Level;
	}

	ApplyChargeLevelToAttack(Level);

	// 게스트는 자기 결과를 서버에 알린다 (서버는 같은 지점에서 멈춰 기다리는 중)
	const FGameplayAbilityActorInfo& Info = GetActorInfo();
	if (Info.IsLocallyControlled() && !Info.IsNetAuthority())
	{
		SendChargeReleaseToServer(Level);
	}

	ExecuteChargeCue(ChargeReleaseCueTag, Level);

	// 단계별 다른 베기 섹션이 있으면 그 섹션으로 이동
	if (Level > 0)
	{
		JumpAttackMontageToSection(ChargeLevels[Level - 1].ReleaseSectionName);
	}

	// 차지하는 동안 바뀐 락온 대상·입력 방향으로 공격 방향을 다시 맞춘다
	UpdateAttackWarpTarget();
	SetAttackMontagePlayRate(1.0f);

	UE_LOG(LogLee, Log, TEXT("[Charge] %s 차지 발사 Level=%d Damage=%.1f"), GetComboNetRole(this), Level, CurrentAttackData.BaseDamage);
}

// 단계 값을 이번 타 데이터에 반영 (다음 타로 넘어가면 PlayComboAttack이 원래 데이터로 덮어쓴다)
void ULeeGameplayAbility_PlayerAttackMelee::ApplyChargeLevelToAttack(int32 Level)
{
	if (!ChargeLevels.IsValidIndex(Level - 1))
	{
		return;
	}

	const FLeeChargeLevelData& LevelData = ChargeLevels[Level - 1];
	CurrentAttackData.BaseDamage *= LevelData.DamageMultiplier;
	CurrentAttackData.DamageTypeTags.AppendTags(LevelData.ExtraDamageTypeTags);
	if (LevelData.bOverrideKnockback)
	{
		CurrentAttackData.Knockback = LevelData.Knockback;
	}
	if (LevelData.bOverrideHitStop)
	{
		CurrentAttackData.HitStop = LevelData.HitStop;
	}

	// 추가 스태미나는 서버에서만 차감 (기존 공격 단계 비용과 같은 GE 사용)
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!GetActorInfo().IsNetAuthority() || !ASC || !AttackStepStaminaCostEffect || LevelData.ExtraStaminaCost <= 0.0f)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(AttackStepStaminaCostEffect, /*Level*/1.0f, ASC->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_StaminaDamage, -LevelData.ExtraStaminaCost);
		SpecHandle.Data->AddDynamicAssetTag(MyTags::Souls::StaminaCause_AttackCost);
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

// 차지 타이머·상태 정리
void ULeeGameplayAbility_PlayerAttackMelee::ResetChargeState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChargeTimerHandle);
	}
	bCharging = false;
	CurrentChargeLevel = 0;
}

// [로컬] ChargeInputAction에 매핑된 키 중 하나라도 지금 눌려 있는지 (입력 트리거 종류와 상관없이 실제 키 상태로 판단)
bool ULeeGameplayAbility_PlayerAttackMelee::IsChargeInputHeld() const
{
	// 26.11.17 수정 - 키 상태 확인은 베이스(ULeeGameplayAbility::IsInputActionHeld)로 옮겨 달리기와 함께 쓴다
	return IsInputActionHeld(ChargeInputAction);
}

// 해당 단계(1부터)의 추가 스태미나를 지금 낼 수 있는지
bool ULeeGameplayAbility_PlayerAttackMelee::CanAffordChargeLevel(int32 Level) const
{
	if (!ChargeLevels.IsValidIndex(Level - 1))
	{
		return Level <= 0;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const float CurrentStamina = ASC ? ASC->GetNumericAttribute(ULeeSoulsStatSet::GetStaminaAttribute()) : 0.0f;
	return CurrentStamina >= ChargeLevels[Level - 1].ExtraStaminaCost;
}

// 마지막 단계 시간 + 자동 발사 지연
float ULeeGameplayAbility_PlayerAttackMelee::GetChargeAutoReleaseTime() const
{
	const float LastLevelTime = ChargeLevels.IsEmpty() ? 0.0f : ChargeLevels.Last().HoldTime;
	return LastLevelTime + AutoReleaseDelay;
}

// 몽타주 재생 속도 변경 — 서버는 ASC 함수로 다른 클라이언트에 복제, 게스트는 자기 화면만 (ASC 함수는 서버 RPC를 보내 서버 쪽 멈춤과 충돌한다)
void ULeeGameplayAbility_PlayerAttackMelee::SetAttackMontagePlayRate(float PlayRate)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	UAnimMontage* Montage = ASC ? ASC->GetCurrentMontage() : nullptr;
	if (!Montage)
	{
		return;
	}

	if (GetActorInfo().IsNetAuthority())
	{
		ASC->CurrentMontageSetPlayRate(PlayRate);
	}
	else if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
	{
		AnimInstance->Montage_SetPlayRate(Montage, PlayRate);
	}
}

// 몽타주 섹션 이동 — 섹션 이름이 비었거나 몽타주에 없으면 아무것도 안 한다(이어서 재생)
void ULeeGameplayAbility_PlayerAttackMelee::JumpAttackMontageToSection(FName SectionName)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	UAnimMontage* Montage = ASC ? ASC->GetCurrentMontage() : nullptr;
	if (SectionName.IsNone() || !Montage || Montage->GetSectionIndex(SectionName) == INDEX_NONE)
	{
		return;
	}

	if (GetActorInfo().IsNetAuthority())
	{
		ASC->CurrentMontageJumpToSection(SectionName);
	}
	else if (UAnimInstance* AnimInstance = GetActorInfo().GetAnimInstance())
	{
		AnimInstance->Montage_JumpToSection(SectionName, Montage);
	}
}

// 차지 Cue 실행 — 게스트는 자기 화면에 바로, 서버는 활성화 예측 키를 실어 보내 게스트 본인은 건너뛰고 다른 클라이언트만 재생시킨다
void ULeeGameplayAbility_PlayerAttackMelee::ExecuteChargeCue(const FGameplayTag& CueTag, int32 Level)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!CueTag.IsValid() || !ASC || !Avatar)
	{
		return;
	}

	FGameplayCueParameters Params;
	Params.Instigator = Avatar;
	Params.EffectCauser = Avatar;
	Params.Location = Avatar->GetActorLocation();
	Params.RawMagnitude = static_cast<float>(Level);

	if (GetActorInfo().IsNetAuthority())
	{
		FScopedPredictionWindow ScopedPrediction(ASC, GetCurrentActivationInfo().GetActivationPredictionKey(), /*bSetReplicatedPredictionKey*/false);
		ASC->ExecuteGameplayCue(CueTag, Params);
	}
	else if (GetActorInfo().IsLocallyControlled())
	{
		// 26.11.16 수정 - UE 5.6 ASC에는 ExecuteGameplayCueLocal이 없다. 복제 없이 이 화면에서만 Executed 이벤트를 실행한다
		ASC->InvokeGameplayCueEvent(CueTag, EGameplayCueEvent::Executed, Params);
	}
}

// [게스트] 차지 결과 단계를 서버로 보낸다 (Payload.X = 단계)
void ULeeGameplayAbility_PlayerAttackMelee::SendChargeReleaseToServer(int32 Level)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FScopedPredictionWindow ScopedPrediction(ASC, true);
	ASC->ServerSetReplicatedEventWithPayload(EAbilityGenericReplicatedEvent::GameCustom2, CurrentSpecHandle,
		GetCurrentActivationInfo().GetActivationPredictionKey(), ASC->ScopedPredictionKey, FVector(static_cast<double>(Level), 0.0, 0.0));
	UE_LOG(LogLee, Log, TEXT("[Charge] %s 서버에 차지 결과 전송 Level=%d"), GetComboNetRole(this), Level);
}

// [서버, 원격 플레이어] 차지 결과 신호 구독 (어빌리티당 1회) + 이미 도착한 신호가 있으면 바로 처리
void ULeeGameplayAbility_PlayerAttackMelee::ListenForRemoteChargeRelease()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	const FPredictionKey ActivationKey = GetCurrentActivationInfo().GetActivationPredictionKey();
	if (!RemoteChargeReleaseHandle.IsValid())
	{
		RemoteChargeReleaseHandle = ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::GameCustom2, CurrentSpecHandle, ActivationKey)
			.AddUObject(this, &ThisClass::OnRemoteChargeRelease);
	}
	ASC->CallReplicatedEventDelegateIfSet(EAbilityGenericReplicatedEvent::GameCustom2, CurrentSpecHandle, ActivationKey);
}

// [서버, 원격 플레이어] 게스트가 보낸 단계로 이어서 벤다. 아직 차지 지점 전이면 소비하지 않고 남겨 둔다(지점 도달 시 다시 호출됨)
void ULeeGameplayAbility_PlayerAttackMelee::OnRemoteChargeRelease()
{
	if (!IsActive() || !bCharging)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	const FPredictionKey ActivationKey = GetCurrentActivationInfo().GetActivationPredictionKey();
	const FAbilityReplicatedData Data = ASC->GetReplicatedDataOfGenericReplicatedEvent(EAbilityGenericReplicatedEvent::GameCustom2, CurrentSpecHandle, ActivationKey);
	ASC->ConsumeGenericReplicatedEvent(EAbilityGenericReplicatedEvent::GameCustom2, CurrentSpecHandle, ActivationKey);

	const int32 Level = FMath::RoundToInt(Data.VectorPayload.X);
	UE_LOG(LogLee, Log, TEXT("[Charge] %s 게스트 차지 결과 수신 Level=%d"), GetComboNetRole(this), Level);
	ReleaseCharge(Level);
}
