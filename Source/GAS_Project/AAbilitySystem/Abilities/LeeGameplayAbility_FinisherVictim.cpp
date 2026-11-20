// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbility_FinisherVictim.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GAS_Project/ACharacter/LeeHealthComponent.h"
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

	const ULeeFinisherVictimData* VictimData = FindVictimData(Avatar);
	UAnimMontage* VictimMontage = VictimData ? VictimData->GetVictimMontage(Type) : nullptr;

	if (!VictimMontage)
	{
		const ULeeFinisherTargetComponent* TargetComponent = Avatar->FindComponentByClass<ULeeFinisherTargetComponent>();
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

	// 26.11.17 추가 - 피해자 몽타주는 Auto Blend Out이 꺼져 있어 끝나도 완료 콜백이 오지 않으므로 몽타주 길이로 종료 시점을 잡는다
	// 26.11.20 수정 - 몽타주 에셋의 Rate Scale(배속)을 반영한 실제 재생 시간으로 기다린다
	const float VictimRateScale = FMath::Max(VictimMontage->RateScale, KINDA_SMALL_NUMBER);
	UAbilityTask_WaitDelay* FinishTask = UAbilityTask_WaitDelay::WaitDelay(this, VictimMontage->GetPlayLength() / VictimRateScale);
	FinishTask->OnFinish.AddDynamic(this, &ThisClass::OnVictimMontageFinished);
	FinishTask->ReadyForActivation();
}

// 대상 액터의 스켈레톤 태그(FinisherTargetComponent)로 레지스트리에서 VictimData를 찾는다
const ULeeFinisherVictimData* ULeeGameplayAbility_FinisherVictim::FindVictimData(const AActor* Victim)
{
	const ULeeFinisherTargetComponent* TargetComponent = Victim ? Victim->FindComponentByClass<ULeeFinisherTargetComponent>() : nullptr;
	const ULeeFinisherVictimRegistry* Registry = ULeeAssetManager::Get().GetFinisherVictimRegistry();
	return (TargetComponent && Registry) ? Registry->FindVictimData(TargetComponent->GetSkeletonTag()) : nullptr;
}

// 현재 섹션의 다음 섹션(Rise) 연결을 끊어 섹션 끝(쓰러진 자세)에서 멈추게 하고, 멈출 때까지 남은 시간(초)을 반환한다
// 서버에서 ASC로 호출하므로 다른 클라이언트에도 복제된다. Auto Blend Out이 꺼진 몽타주여야 끝 자세가 유지된다
float ULeeGameplayAbility_FinisherVictim::HoldDownedPose(UAbilitySystemComponent* ASC)
{
	UAnimInstance* AnimInstance = (ASC && ASC->AbilityActorInfo.IsValid()) ? ASC->AbilityActorInfo->GetAnimInstance() : nullptr;
	UAnimMontage* Montage = ASC ? ASC->GetCurrentMontage() : nullptr;
	if (!AnimInstance || !Montage)
	{
		return 0.0f;
	}

	const float Position = AnimInstance->Montage_GetPosition(Montage);
	const int32 SectionIndex = Montage->GetSectionIndexFromPosition(Position);
	if (SectionIndex == INDEX_NONE)
	{
		return 0.0f;
	}

	ASC->CurrentMontageSetNextSectionName(Montage->GetSectionName(SectionIndex), NAME_None);

	float SectionStartTime = 0.0f;
	float SectionEndTime = 0.0f;
	Montage->GetSectionStartAndEndTime(SectionIndex, SectionStartTime, SectionEndTime);

	// 26.11.20 수정 - 실제 재생 속도 = 재생 요청 배속 × 몽타주 에셋 Rate Scale
	const float PlayRate = FMath::Max(AnimInstance->Montage_GetPlayRate(Montage) * Montage->RateScale, KINDA_SMALL_NUMBER);
	return FMath::Max(SectionEndTime - Position, 0.0f) / PlayRate;
}

// 몽타주 길이만큼 지난 뒤 호출 — 살아 있으면 종료(몽타주 블렌드 아웃), 죽었으면 쓰러진 자세를 유지하도록 그대로 둔다
void ULeeGameplayAbility_FinisherVictim::OnVictimMontageFinished()
{
	const ULeeHealthComponent* HealthComponent = ULeeHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo());
	if (HealthComponent && HealthComponent->IsDeadOrDying())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}

void ULeeGameplayAbility_FinisherVictim::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}

void ULeeGameplayAbility_FinisherVictim::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/true);
}
