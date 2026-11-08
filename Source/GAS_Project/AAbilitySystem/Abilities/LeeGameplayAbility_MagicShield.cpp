// 26.11.08 신규 - 마법 Enemy(모르가나 등)의 가드 대신 쓰는 마법 쉴드

#include "LeeGameplayAbility_MagicShield.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GAS_Project/MyTags.h"

// 생성자 — 서버 실행, 다른 공격과 함께 켜질 수 있게 Independent, 쉴드 태그는 활성 동안 자동 부여
ULeeGameplayAbility_MagicShield::ULeeGameplayAbility_MagicShield(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup    = ELeeAbilityActivationGroup::Independent;

	// StateTree Lee Activate Ability Task가 이 태그로 어빌리티를 찾는다
	FGameplayTagContainer AssetTagContainer;
	AssetTagContainer.AddTag(MyTags::Souls::Ability_MagicShield);
	SetAssetTags(AssetTagContainer);

	// ExecCalc가 이 태그로 피해 흡수를 판정하고, 투사체 공격이 이 태그로 막힌다
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Shield_Active);

	// 그로기 중이거나 재사용 대기 중에는 켤 수 없다
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Groggy);
	ActivationBlockedTags.AddTag(MyTags::Souls::Cooldown_MagicShield);
}

// 활성화 — 쉴드 HP 초기화, 쉴드 표시 Cue 시작, 작동 시간/피격 이벤트 대기
void ULeeGameplayAbility_MagicShield::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentShieldHP = ShieldMaxHP;
	bShieldBroken = false;
	LastAttacker = nullptr;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddGameplayCue(MyTags::Souls::GameplayCue_Shield_Active, ASC->MakeEffectContext());
	}

	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, MyTags::Souls::Event_Defense_ShieldHit, /*OptionalExternalOwner*/nullptr, /*OnlyTriggerOnce*/false, /*OnlyMatchExact*/true);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnShieldHitEventReceived);
	HitTask->ReadyForActivation();

	UAbilityTask_WaitDelay* DurationTask = UAbilityTask_WaitDelay::WaitDelay(this, ShieldDuration);
	DurationTask->OnFinish.AddDynamic(this, &ThisClass::OnShieldDurationFinished);
	DurationTask->ReadyForActivation();
}

// 종료 — 쉴드 표시 Cue 제거 + 재사용 대기 적용. 깨져서 끝났으면 종료 뒤 짧은 경직 이벤트를 보낸다
void ULeeGameplayAbility_MagicShield::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 작동 시간 종료와 쉴드 파괴가 같은 프레임에 겹쳐도 정리는 한 번만 한다
	if (!IsActive())
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC)
	{
		ASC->RemoveGameplayCue(MyTags::Souls::GameplayCue_Shield_Active);
	}

	if (ShieldCooldown > 0.0f)
	{
		if (ShieldCooldownEffect)
		{
			FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(ShieldCooldownEffect, GetAbilityLevel());
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Duration, ShieldCooldown);
				ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
			}
		}
		else if (ASC)
		{
			// GE를 지정하지 않으면 Loose 태그 + 타이머로 재사용 대기를 건다 (AI는 서버에서만 판단하므로 복제 불필요)
			ASC->AddLooseGameplayTag(MyTags::Souls::Cooldown_MagicShield);
			TWeakObjectPtr<UAbilitySystemComponent> WeakASC = ASC;
			FTimerHandle CooldownTimer;
			ASC->GetWorld()->GetTimerManager().SetTimer(CooldownTimer, FTimerDelegate::CreateLambda([WeakASC]()
			{
				if (UAbilitySystemComponent* CooldownASC = WeakASC.Get())
				{
					CooldownASC->RemoveLooseGameplayTag(MyTags::Souls::Cooldown_MagicShield);
				}
			}), ShieldCooldown, false);
		}
	}

	const bool bSendStun = bShieldBroken;
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	AActor* StunInstigator = LastAttacker.Get();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// 어빌리티가 끝난 뒤에 보내야 HitReaction이 바로 활성화된다
	if (bSendStun && Avatar)
	{
		FGameplayEventData Payload;
		Payload.EventTag = MyTags::Souls::Event_Combat_HitReact;
		Payload.Target = Avatar;
		Payload.Instigator = StunInstigator; // 리액션 방향(모션워핑) 계산용 — DefenseComponent의 HitReact 발송과 같은 규칙
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar, MyTags::Souls::Event_Combat_HitReact, Payload);
	}
}

// 쉴드 피격 — 피해만큼 쉴드 HP를 깎고(넘친 피해는 버림) 금 가는 Cue, 0이면 깨짐
void ULeeGameplayAbility_MagicShield::OnShieldHitEventReceived(FGameplayEventData Payload)
{
	if (!IsActive() || bShieldBroken)
	{
		return;
	}

	LastAttacker = const_cast<AActor*>(Payload.Instigator.Get());
	CurrentShieldHP = FMath::Max(0.0f, CurrentShieldHP - Payload.EventMagnitude);

	// 26.11.08 추가 - 맞은 방향: 쉴드 중심에서 공격자 쪽 (공격자를 모르면 정면)
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	FVector HitDirection = Avatar ? Avatar->GetActorForwardVector() : FVector::ForwardVector;
	if (Avatar && LastAttacker.IsValid())
	{
		HitDirection = (LastAttacker->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal(UE_SMALL_NUMBER, HitDirection);
	}
	ExecuteShieldHitCue(CurrentShieldHP / ShieldMaxHP, HitDirection);

	if (CurrentShieldHP <= 0.0f)
	{
		bShieldBroken = true;
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
	}
}

// 작동 시간 종료 — 쉴드를 조용히 내린다
void ULeeGameplayAbility_MagicShield::OnShieldDurationFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}

// 피격 Cue 실행 — RawMagnitude에 남은 쉴드 비율, Normal에 맞은 방향을 넣는다 (Cue가 금 위치·크기와 파괴 여부를 정한다)
void ULeeGameplayAbility_MagicShield::ExecuteShieldHitCue(float RemainingRatio, const FVector& HitDirection) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	FGameplayCueParameters Params;
	Params.RawMagnitude = FMath::Clamp(RemainingRatio, 0.0f, 1.0f);
	Params.Instigator = GetAvatarActorFromActorInfo();
	Params.Location = GetAvatarActorFromActorInfo() ? GetAvatarActorFromActorInfo()->GetActorLocation() : FVector::ZeroVector;
	Params.Normal = HitDirection;
	ASC->ExecuteGameplayCue(MyTags::Souls::GameplayCue_Shield_Hit, Params);
}
