#include "LeeGameplayAbility_Guard.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACharacter/LeeTargetLockComponent.h"

// 생성자 — Hold형 입력 정책 + 태그/그룹 설정
ULeeGameplayAbility_Guard::ULeeGameplayAbility_Guard(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationPolicy   = ELeeAbilityActivationPolicy::WhileInputActive;
	ActivationGroup    = ELeeAbilityActivationGroup::Independent;

	// 어빌리티 식별 태그 — DefenseComponent가 가드 브레이크 시 이 태그로 강제 취소한다
	AbilityTags.AddTag(MyTags::Souls::Ability_Guard);

	// 가드 상태 태그 — 활성 동안 자동 부여, 종료 시 자동 제거 (ABP GuardState 전환용)
	// 주의: RegenBlocked는 넣지 않는다 — 가드 유지 중에도 스태미나가 회복
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Guard_Active);

	// 회피 중/그로기 중에는 가드 시작 불가
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Dodge_Active);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Groggy);
}

// 활성화 — 퍼펙트 윈도우 GE 적용 + 판정 이벤트/입력 해제 대기 등록
void ULeeGameplayAbility_Guard::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 가드는 시작 시 스태미나를 소모하지 않는다 (피격 시에만 ExecCalc가 소모)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 퍼펙트 가드 윈도우 — Duration GE가 만료되면 Guard.Perfect 태그 자동 제거
	PerfectGuardWindowHandle = ApplyDurationEffect(PerfectGuardWindowEffect, PerfectGuardWindow);

	// 2. 판정 이벤트 대기 (반복 수신 — 가드 유지 중 여러 번 맞을 수 있음)
	UAbilityTask_WaitGameplayEvent* GuardHitTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, MyTags::Souls::Event_Defense_GuardHit,
			/*OptionalExternalOwner*/nullptr, /*OnlyTriggerOnce*/false, /*OnlyMatchExact*/true);
	GuardHitTask->EventReceived.AddDynamic(this, &ThisClass::OnGuardHitEventReceived);
	GuardHitTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* PerfectGuardTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, MyTags::Souls::Event_Defense_PerfectGuard,
			/*OptionalExternalOwner*/nullptr, /*OnlyTriggerOnce*/false, /*OnlyMatchExact*/true);
	PerfectGuardTask->EventReceived.AddDynamic(this, &ThisClass::OnPerfectGuardEventReceived);
	PerfectGuardTask->ReadyForActivation();

	// 3. 입력 해제 대기 — WhileInputActive는 재활성화 정책일 뿐 종료를 대신하지 않으므로 필요 
	UAbilityTask_WaitInputRelease* ReleaseTask =
		UAbilityTask_WaitInputRelease::WaitInputRelease(this, /*bTestAlreadyReleased*/true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnInputReleased);
	ReleaseTask->ReadyForActivation();
}

// 종료 — 퍼펙트 윈도우 GE 핸들 정리 (Guard.Active는 ActivationOwnedTags라 자동 제거)
void ULeeGameplayAbility_Guard::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 윈도우 GE가 아직 만료 전이면 명시적으로 제거 — 가드 온오프 반복 시 중복 잔류 방지
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC && PerfectGuardWindowHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(PerfectGuardWindowHandle);
	}
	PerfectGuardWindowHandle.Invalidate();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// 일반 가드 피격 — 플린치 몽타주 재생(스탠스별 섹션 분기), 가드는 유지
void ULeeGameplayAbility_Guard::OnGuardHitEventReceived(FGameplayEventData /*Payload*/)
{
	PlayGuardMontageWithStance(GuardFlinchMontage);
}

// 퍼펙트 가드 — 패리 몽타주 재생(스탠스별 섹션 분기) + 반격 윈도우 GE 적용 (공격자 처리는 DefenseComponent 담당)
void ULeeGameplayAbility_Guard::OnPerfectGuardEventReceived(FGameplayEventData /*Payload*/)
{
	PlayGuardMontageWithStance(GuardParryMontage);

	// 반격 윈도우는 가드 종료 후에도 남아야 하므로 핸들을 보관하지 않는다 (Duration 만료에 맡김)
	ApplyDurationEffect(CounterWindowEffect, CounterWindowDuration);
}

// 가드 스탠스(왼발 뒤/오른발 뒤)에 맞는 섹션 이름을 고른다.
// 판정값은 ULeeTargetLockComponent가 소유 — ABP가 고르는 가드 포즈와 동일 소스라 절대 어긋나지 않는다
FName ULeeGameplayAbility_Guard::SelectStanceSection(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return NAME_None;
	}

	const AActor* Avatar = GetAvatarActorFromActorInfo();
	const ULeeTargetLockComponent* LockComp = ULeeTargetLockComponent::FindTargetLockComponent(Avatar);
	const bool bLeftFootBack = LockComp && LockComp->IsGuardLeftFootBack();

	const FName Desired = bLeftFootBack ? StanceLeftFootSection : StanceRightFootSection;

	// 섹션이 아직 안 만들어진 몽타주여도 가드는 계속 유지되어야 하므로, 어빌리티를 끊지 않고 처음부터 재생한다
	if (Montage->GetSectionIndex(Desired) == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_Guard] %s에 Section [%s]이 없습니다. 처음부터 재생합니다."),
			*Montage->GetName(), *Desired.ToString());
		return NAME_None;
	}
	return Desired;
}

// 몽타주 재생 공통 헬퍼 — 플린치/패리가 동일한 스탠스 섹션 규약을 공유한다
void ULeeGameplayAbility_Guard::PlayGuardMontageWithStance(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, Montage, /*PlayRate*/1.0f, SelectStanceSection(Montage));
	MontageTask->ReadyForActivation();
}

// 입력 해제 — 가드 종료
void ULeeGameplayAbility_Guard::OnInputReleased(float /*TimeHeld*/)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}

// SetByCaller Duration 방식 GE 적용 헬퍼 — BP 애셋의 Duration이 SetByCaller(Souls.SetByCaller.Duration)여야 한다
FActiveGameplayEffectHandle ULeeGameplayAbility_Guard::ApplyDurationEffect(
	TSubclassOf<UGameplayEffect> EffectClass, float Duration)
{
	if (!EffectClass || Duration <= 0.0f)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(EffectClass, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Duration, Duration);
	return ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
}
