#include "LeeGameplayAbility_HitReaction.h"

#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "MotionWarpingComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS_Project/MyTags.h"

// 생성자 — GameplayEvent 트리거 4종 등록 + 그룹/재트리거 설정
ULeeGameplayAbility_HitReaction::ULeeGameplayAbility_HitReaction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup    = ELeeAbilityActivationGroup::Exclusive_Replaceable;

	AbilityTags.AddTag(MyTags::Souls::Ability_HitReaction);

	// 리액션 중 새 리액션 이벤트가 오면 기존 인스턴스를 재활성화 (예: 패리당함 직후 체간 붕괴)
	bRetriggerInstancedAbility = true;

	// 트리거 이벤트 등록 — 이 태그의 GameplayEvent가 오면 자동 활성화
	const FGameplayTag TriggerTags[] = {
		MyTags::Souls::Event_Combat_Parried,
		MyTags::Souls::Event_Combat_HitReact,
		MyTags::Souls::Event_Combat_GuardBreak,
		MyTags::Souls::Event_Combat_PostureBreak,
	};
	for (const FGameplayTag& Tag : TriggerTags)
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = Tag;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

// 활성화 — 트리거 태그로 몽타주 선택, 패리당함이면 방어자 방향 모션워핑 설정
void ULeeGameplayAbility_HitReaction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar || !TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimMontage* SelectedMontage = SelectMontageForEvent(TriggerEventData->EventTag);
	if (!SelectedMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_HitReaction] %s에 해당하는 리액션 몽타주가 설정되지 않음. BP에서 지정해주세요."),
			*TriggerEventData->EventTag.ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FName StartSection = NAME_None;
	if (TriggerEventData->EventTag == MyTags::Souls::Event_Combat_HitReact)
	{
		const AActor* Attacker = TriggerEventData->Instigator.Get();
		StartSection = SelectStaggerSection(Avatar, Attacker);

		if (SelectedMontage->GetSectionIndex(StartSection) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("[LeeGA_HitReaction] StaggerReactionMontage에 Section [%s]이 없습니다."),
				*StartSection.ToString());
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}

	// 패리당함: 방어자(Instigator) 방향을 바라보도록 회전 워프 (GA_Finisher WarpTarget 패턴 재사용)
	if (TriggerEventData->EventTag == MyTags::Souls::Event_Combat_Parried)
	{
		if (const AActor* Defender = TriggerEventData->Instigator.Get())
		{
			if (UMotionWarpingComponent* MotionWarping = Avatar->FindComponentByClass<UMotionWarpingComponent>())
			{
				const FVector ToDefender =
					(Defender->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal2D();
				MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(
					WarpTargetName, Avatar->GetActorLocation(), ToDefender.Rotation());
			}
		}
	}

	// 몽타주 재생 — 모든 종료 경로가 EndAbility로 수렴
	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, SelectedMontage, /*PlayRate*/1.0f, StartSection);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->ReadyForActivation();
}

// 종료 — 모션워핑 타겟 정리
void ULeeGameplayAbility_HitReaction::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr)
	{
		if (UMotionWarpingComponent* MotionWarping = Avatar->FindComponentByClass<UMotionWarpingComponent>())
		{
			MotionWarping->RemoveWarpTarget(WarpTargetName);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// 트리거 이벤트 태그 → 리액션 몽타주 매핑
UAnimMontage* ULeeGameplayAbility_HitReaction::SelectMontageForEvent(const FGameplayTag& EventTag) const
{
	if (EventTag == MyTags::Souls::Event_Combat_Parried)
	{
		return ParriedReactionMontage;
	}
	if (EventTag == MyTags::Souls::Event_Combat_GuardBreak)
	{
		return GuardBreakReactionMontage;
	}
	if (EventTag == MyTags::Souls::Event_Combat_PostureBreak)
	{
		return GroggyReactionMontage;
	}
	if (EventTag == MyTags::Souls::Event_Combat_HitReact)
	{
		return StaggerReactionMontage;
	}
	return nullptr;
}

// 몽타주 종료(완료/블렌드아웃/인터럽트/취소) — 어빌리티 종료
FName ULeeGameplayAbility_HitReaction::SelectStaggerSection(const AActor* Avatar, const AActor* Attacker) const
{
	if (!Avatar || !Attacker)
	{
		return StaggerFrontSection;
	}

	FVector ToAttacker = Attacker->GetActorLocation() - Avatar->GetActorLocation();
	ToAttacker.Z = 0.0f;
	if (ToAttacker.IsNearlyZero())
	{
		return StaggerFrontSection;
	}
	ToAttacker.Normalize();

	FVector Forward = Avatar->GetActorForwardVector();
	Forward.Z = 0.0f;
	Forward.Normalize();

	FVector Right = Avatar->GetActorRightVector();
	Right.Z = 0.0f;
	Right.Normalize();

	const float ForwardAmount = FVector::DotProduct(ToAttacker, Forward);
	const float RightAmount = FVector::DotProduct(ToAttacker, Right);

	if (FMath::Abs(ForwardAmount) >= FMath::Abs(RightAmount))
	{
		return ForwardAmount >= 0.0f ? StaggerFrontSection : StaggerBackSection;
	}

	return RightAmount >= 0.0f ? StaggerRightSection : StaggerLeftSection;
}

void ULeeGameplayAbility_HitReaction::OnMontageFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}
