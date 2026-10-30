#include "LeeGameplayAbility_DrinkEstus.h"

#include "Animation/AnimMontage.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"

// 생성자 — 입력 트리거 / Replaceable 그룹 / 마시는 중 차단할 행동 설정
ULeeGameplayAbility_DrinkEstus::ULeeGameplayAbility_DrinkEstus(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationPolicy   = ELeeAbilityActivationPolicy::OnInputTriggered;
	// 피격 시 HitReaction(같은 그룹)이 이 어빌리티를 자동 취소한다
	ActivationGroup    = ELeeAbilityActivationGroup::Exclusive_Replaceable;

	AbilityTags.AddTag(MyTags::Souls::Ability_Estus);

	// 마시는 중 상태 태그 (이동 감속 판정) — 종료 시 자동 제거
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Estus_Drinking);

	// 회피·가드·그로기·사망 중에는 마실 수 없다
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Dodge_Active);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Guard_Active);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Groggy);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Death_Dying);

	// 마시는 동안 구르기·공격·가드·처형 입력 차단 (소울라이크 방식)
	BlockAbilitiesWithTag.AddTag(MyTags::Souls::Ability_Dodge);
	BlockAbilitiesWithTag.AddTag(MyTags::Souls::Ability_Guard);
	BlockAbilitiesWithTag.AddTag(MyTags::Souls::Ability_Attack_Melee_Light);
	BlockAbilitiesWithTag.AddTag(MyTags::Souls::Ability_Attack_Melee_Heavy);
	BlockAbilitiesWithTag.AddTag(MyTags::Souls::Status_Attack_Melee);
	BlockAbilitiesWithTag.AddTag(MyTags::Souls::Ability_Execution);
	BlockAbilitiesWithTag.AddTag(MyTags::Souls::Ability_Assassination);
}

// 활성화 — 횟수 소모(Commit) → 몽타주 재생 + 회복 이벤트 대기
void ULeeGameplayAbility_DrinkEstus::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 몽타주가 없으면 횟수를 쓰기 전에 종료
	if (!DrinkMontage)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeGA_DrinkEstus] DrinkMontage가 설정되지 않음. BP에서 지정해주세요."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Cost GE(EstusCharges -1)는 BP에서 지정 — 남은 횟수가 0이면 여기서 실패
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 회복 시점 이벤트 대기 (몽타주의 AnimNotify가 발사)
	UAbilityTask_WaitGameplayEvent* HealEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, MyTags::Souls::Event_Estus_Heal,
			/*OptionalExternalOwner*/nullptr, /*OnlyTriggerOnce*/true, /*OnlyMatchExact*/true);
	HealEventTask->EventReceived.AddDynamic(this, &ThisClass::OnHealEventReceived);
	HealEventTask->ReadyForActivation();

	// 2. 마시는 몽타주 재생
	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, DrinkMontage);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
}

// 회복 시점 도달 — 회복 GE 적용 (이 전에 피격되면 어빌리티가 취소되어 호출되지 않음)
void ULeeGameplayAbility_DrinkEstus::OnHealEventReceived(FGameplayEventData Payload)
{
	if (!HealEffect)
	{
		UE_LOG(LogLee, Warning, TEXT("[LeeGA_DrinkEstus] HealEffect가 설정되지 않음. BP에서 GE_EstusHeal을 지정해주세요."));
		return;
	}

	const FGameplayEffectSpecHandle HealSpec = MakeOutgoingGameplayEffectSpec(HealEffect, GetAbilityLevel());
	ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, HealSpec);
}

// 몽타주 정상 완료
void ULeeGameplayAbility_DrinkEstus::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}

// 몽타주 인터럽트/외부 취소 (피격 등) — 횟수는 이미 소모된 상태
void ULeeGameplayAbility_DrinkEstus::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/true);
}
