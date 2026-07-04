// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbility_FinisherVictim.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACharacter/LeeFinisherTargetComponent.h"
#include "GAS_Project/ACharacter/LeeFinisherVictimData.h"
#include "GAS_Project/ACharacter/LeeFinisherVictimRegistry.h"
#include "GAS_Project/System/LeeAssetManager.h"
#include "LeeGameplayAbility_Finisher.h"

ULeeGameplayAbility_FinisherVictim::ULeeGameplayAbility_FinisherVictim(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup    = ELeeAbilityActivationGroup::Exclusive_Blocking;

	AbilityTags.AddTag(MyTags::Souls::Ability_AssassinationVictim);
	// 처형 데미지로 HP가 0이 되는 순간 GA_Death가 CancelAbilities()를 호출하는데,
	// 이 태그가 없으면 몽타주 재생 중에도 강제로 취소된다 — 몽타주를 끝까지 재생하려면 필수.
	AbilityTags.AddTag(MyTags::Souls::Ability_Behavior_SurvivesDeath);

	// 당하는 동안 자동 부여/해제 — 중복 피니셔 잠금 겸 AI 반응용
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Finisher_Victim);
	// 시퀀스 동안 피해자 이동/회전 완전 정지 (LeeCharacterMovementComponent가 이 태그를 감지해 MaxSpeed=0 처리)
	ActivationOwnedTags.AddTag(MyTags::Souls::Gameplay_MovementStopped);

	// 진행 중이던 근접 공격을 끊고, 시퀀스 동안 새 공격을 차단
	CancelAbilitiesWithTag.AddTag(MyTags::Souls::Status_Attack_Melee);
	BlockAbilitiesWithTag.AddTag(MyTags::Souls::Status_Attack_Melee);

	// GA_Finisher가 보내는 이벤트로 자동 활성화
	FAbilityTriggerData Trigger;
	Trigger.TriggerTag = MyTags::Souls::Event_BeFinished;
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);
}

void ULeeGameplayAbility_FinisherVictim::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	const AActor* Attacker = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;

	if (!Avatar || !Attacker || !TriggerEventData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_FinisherVictim] 이벤트 페이로드가 불완전합니다 (Instigator 확인)."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 몽타주는 공격자가 아니라 피해자 자신의 스켈레톤 태그로 조회한다
	const ELeeFinisherType Type = static_cast<ELeeFinisherType>(TriggerEventData->EventMagnitude);

	const ULeeFinisherTargetComponent* TargetComponent = Avatar->FindComponentByClass<ULeeFinisherTargetComponent>();
	const ULeeFinisherVictimRegistry* Registry = ULeeAssetManager::Get().GetFinisherVictimRegistry();
	const ULeeFinisherVictimData* VictimData = (TargetComponent && Registry)
		? Registry->FindVictimData(TargetComponent->GetSkeletonTag())
		: nullptr;
	UAnimMontage* VictimMontage = VictimData ? VictimData->GetVictimMontage(Type) : nullptr;

	if (!VictimMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_FinisherVictim] %s의 스켈레톤 태그(%s)에 해당하는 VictimData/몽타주를 찾지 못했습니다. LeeFinisherVictimRegistry 설정을 확인해주세요."),
			*GetNameSafe(Avatar),
			TargetComponent ? *TargetComponent->GetSkeletonTag().ToString() : TEXT("<NoComponent>"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 이동 정지 + 공격자와 동일한 공식으로 회전 스냅 (몽타주 정렬 동기화)
	const float VictimYaw = ULeeGameplayAbility_Finisher::ComputeVictimYaw(
		Type, Attacker->GetActorLocation(), Avatar->GetActorLocation());

	if (ACharacter* AvatarCharacter = Cast<ACharacter>(Avatar))
	{
		AvatarCharacter->GetCharacterMovement()->StopMovementImmediately();
	}
	Avatar->SetActorRotation(FRotator(0.0f, VictimYaw, 0.0f));

	// 2. 당하는 몽타주 재생 — 모든 종료 경로가 EndAbility로 수렴 (태그 자동 정리)
	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, VictimMontage, /*PlayRate*/1.0f);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
}

void ULeeGameplayAbility_FinisherVictim::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}

void ULeeGameplayAbility_FinisherVictim::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/true);
}
