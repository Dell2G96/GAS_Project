#include "LeeGameplayAbility_HitReaction.h"

#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "MotionWarpingComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystemComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACharacter/LeeDefenseComponent.h"

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
		MyTags::Souls::Event_Combat_HitReactHeavy,
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

	// 몽타주 종료 시 후속 처리(가드브레이크→그로기 체인) 분기에 쓰므로 트리거 태그를 보관한다
	ActiveEventTag = TriggerEventData->EventTag;

	UAnimMontage* SelectedMontage = SelectMontageForEvent(TriggerEventData->EventTag);
	if (!SelectedMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_HitReaction] %s에 해당하는 리액션 몽타주가 설정되지 않음. BP에서 지정해주세요."),
			*TriggerEventData->EventTag.ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FName StartSection = NAME_None;
	if (TriggerEventData->EventTag == MyTags::Souls::Event_Combat_HitReact
		|| TriggerEventData->EventTag == MyTags::Souls::Event_Combat_HitReactHeavy)
	{
		const AActor* Attacker = TriggerEventData->Instigator.Get();
		StartSection = SelectStaggerSection(Avatar, Attacker);

		if (SelectedMontage->GetSectionIndex(StartSection) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("[LeeGA_HitReaction] %s에 Section [%s]이 없습니다."),
				*SelectedMontage->GetName(), *StartSection.ToString());
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

	// 몽타주 재생 — 모든 종료 경로가 OnMontageFinished로 수렴
	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, SelectedMontage, /*PlayRate*/1.0f, StartSection);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageFinished);
	MontageTask->ReadyForActivation();

	// 그로기 몽타주는 loop 섹션이라 스스로 끝나지 않는다.
	// 종료 기준을 "그로기 GE 만료(Status.Groggy 제거)"로 잡고, 그때 어빌리티를 끝내 몽타주를 끊는다.
	// (PlayMontageAndWait는 bStopWhenAbilityEnds가 기본 true이므로 EndAbility만으로 몽타주가 멈춘다)
	if (ActiveEventTag == MyTags::Souls::Event_Combat_PostureBreak)
	{
		UAbilityTask_WaitGameplayTagRemoved* GroggyEndTask =
			UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
				this, MyTags::Souls::Status_Groggy, /*ExternalTarget*/nullptr, /*OnlyTriggerOnce*/true);
		GroggyEndTask->Removed.AddDynamic(this, &ThisClass::OnGroggyTagRemoved);
		GroggyEndTask->ReadyForActivation();
	}
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
	if (EventTag == MyTags::Souls::Event_Combat_HitReactHeavy)
	{
		return HeavyStaggerReactionMontage;
	}
	return nullptr;
}

// 공격자의 상대 방향(전/후/좌/우)으로 재생할 경직 몽타주 섹션 이름을 고른다
FName ULeeGameplayAbility_HitReaction::SelectStaggerSection(const AActor* Avatar, const AActor* Attacker) const
{
	// UE_LOG(LogTemp, Warning, TEXT("[임시디버그][Stagger] Avatar=%s Attacker=%s"),
	// 	*GetNameSafe(Avatar), *GetNameSafe(Attacker));

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

	// // [임시 디버그] 방향 내적값 — Fwd>0=정면, <0=후면 / Right>0=우측, <0=좌측
	// UE_LOG(LogTemp, Warning, TEXT("[임시디버그][Stagger] Fwd=%.2f Right=%.2f"), ForwardAmount, RightAmount);

	if (FMath::Abs(ForwardAmount) >= FMath::Abs(RightAmount))
	{
		return ForwardAmount >= 0.0f ? StaggerFrontSection : StaggerBackSection;
	}

	return RightAmount >= 0.0f ? StaggerRightSection : StaggerLeftSection;
}

// 몽타주 종료(완료/블렌드아웃/인터럽트/취소) — 가드브레이크면 그로기로 체인, 그로기면 태그 만료까지 유지
void ULeeGameplayAbility_HitReaction::OnMontageFinished()
{
	// 그로기 몽타주(loop)는 몽타주 이벤트로 끝내지 않는다. Status.Groggy가 제거될 때만 종료한다.
	// (loop 도중 블렌드아웃 콜백이 와도 그로기 상태가 유지되는 동안에는 어빌리티를 살려둬야 한다)
	if (ActiveEventTag == MyTags::Souls::Event_Combat_PostureBreak)
	{
		return;
	}

	// 선행 리액션(가드브레이크/패리당함) 몽타주가 끝난 시점에 비로소 그로기(처형 가능) 진입.
	// 몽타주가 중간에 끊긴 경우에도 호출해야 상태 누락(연출은 끝났는데 그로기가 아닌 상태)이 생기지 않는다.
	if (ActiveEventTag == MyTags::Souls::Event_Combat_GuardBreak
		|| ActiveEventTag == MyTags::Souls::Event_Combat_Parried)
	{
		if (AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr)
		{
			if (ULeeDefenseComponent* DefenseComp = Avatar->FindComponentByClass<ULeeDefenseComponent>())
			{
				// 연출 중 스태미나가 고갈돼 그로기가 예약돼 있었다면 여기서 실제 진입이 일어난다
				DefenseComp->NotifyReactionFinished(nullptr);

				// 그로기 진입 → PostureBreak 이벤트 → 이 어빌리티 재트리거가 동기적으로 일어난다.
				// 재트리거가 성사됐다면 ActivateAbility가 ActiveEventTag를 PostureBreak로 바꿔놓았으므로,
				// 여기서 EndAbility를 부르면 방금 시작된 그로기 몽타주를 끊게 된다. 반드시 빠져나간다.
				// (클라이언트, 이미 그로기, 예약된 그로기 없음인 경우엔 아래에서 정상 종료)
				if (ActiveEventTag == MyTags::Souls::Event_Combat_PostureBreak)
				{
					return;
				}
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}

// 그로기 GE 만료 — 어빌리티 종료. PlayMontageAndWait가 loop 몽타주를 같이 정리한다
void ULeeGameplayAbility_HitReaction::OnGroggyTagRemoved()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}
