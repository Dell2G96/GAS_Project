#include "LeeGameplayAbility_Sprint.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"

// 생성자 — 태그/그룹/정책 설정
ULeeGameplayAbility_Sprint::ULeeGameplayAbility_Sprint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	// 입력 태그 없이 회피 어빌리티가 켠다. 공격·회피·가드 등 다른 Exclusive 어빌리티가 켜지면 취소된다
	ActivationPolicy   = ELeeAbilityActivationPolicy::OnInputTriggered;
	ActivationGroup    = ELeeAbilityActivationGroup::Exclusive_Replaceable;

	AbilityTags.AddTag(MyTags::Souls::Ability_Sprint);

	// 달리는 중 상태(이동 속도·회전) + 스태미나 회복 정지 (종료 시 자동 제거)
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Sprinting);
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Stamina_RegenBlocked);

	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Groggy);

	// 가드·에스트는 Independent 그룹이라 달리기를 취소하지 않으므로, 이 상태가 되면 달리기가 스스로 끝난다
	StopSprintTags.AddTag(MyTags::Souls::Status_Guard_Active);
	StopSprintTags.AddTag(MyTags::Souls::Status_Estus_Drinking);
	StopSprintTags.AddTag(MyTags::Souls::Status_Groggy);
	StopSprintTags.AddTag(MyTags::Souls::Status_Downed);
}

// 활성화 — 스태미나가 남아 있으면 확인 타이머 시작
void ULeeGameplayAbility_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (GetCurrentStamina() <= 0.0f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(TickTimerHandle, this, &ThisClass::TickSprint, TickInterval, /*bLoop*/true);
	UE_LOG(LogLee, Log, TEXT("[Sprint] 시작 (Authority=%d)"), ActorInfo->IsNetAuthority());
}

// 0.1초마다 — 키를 뗐거나 스태미나가 0이면 끝내고, 서버는 움직이는 동안 스태미나를 뺀다
void ULeeGameplayAbility_Sprint::TickSprint()
{
	if (!IsActive())
	{
		return;
	}

	const FGameplayAbilityActorInfo& Info = GetActorInfo();

	// [직접 조종] 키를 뗐으면 끝 (서버의 원격 플레이어 인스턴스는 게스트의 종료 복제로 끝난다)
	if (Info.IsLocallyControlled() && !IsInputActionHeld(SprintInputAction))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (GetCurrentStamina() <= 0.0f)
	{
		UE_LOG(LogLee, Log, TEXT("[Sprint] 스태미나 소진 → 종료"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 가드·에스트 등 달리기를 멈춰야 하는 상태
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasAnyMatchingGameplayTags(StopSprintTags))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// [서버] 실제로 움직이는 중일 때만 스태미나 차감
	const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	const bool bMoving = Pawn && Pawn->GetVelocity().Size2D() >= MinMoveSpeedForCost;
	if (!Info.IsNetAuthority() || !bMoving || !ASC || !StaminaCostEffect || StaminaCostPerSecond <= 0.0f)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(StaminaCostEffect, /*Level*/1.0f, ASC->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_StaminaDamage, -StaminaCostPerSecond * TickInterval);
		// 원인 태그 — 이 소모로 0이 되어도 그로기에 빠지지 않게 한다 (LeeDefenseComponent::HandleOutOfStamina)
		SpecHandle.Data->AddDynamicAssetTag(MyTags::Souls::StaminaCause_Sprint);
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

// 현재 스태미나 값
float ULeeGameplayAbility_Sprint::GetCurrentStamina() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	return ASC ? ASC->GetNumericAttribute(ULeeSoulsStatSet::GetStaminaAttribute()) : 0.0f;
}

// 종료 — 타이머 정리 + 스태미나 회복 지연
void ULeeGameplayAbility_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TickTimerHandle);
	}

	// ActivationOwnedTags의 RegenBlocked는 종료와 함께 사라지므로, 종료 후 N초 동안 회복 정지를 이어 간다 (회피와 같은 방식)
	if (StaminaRegenDelayEffect && StaminaRegenDelayDuration > 0.0f)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(StaminaRegenDelayEffect, GetAbilityLevel());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Duration, StaminaRegenDelayDuration);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}

	UE_LOG(LogLee, Log, TEXT("[Sprint] 종료 (Cancelled=%d)"), bWasCancelled);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
