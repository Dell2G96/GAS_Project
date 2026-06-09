// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbility_AttackMelee.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GAS_Project/MyTags.h"

ULeeGameplayAbility_AttackMelee::ULeeGameplayAbility_AttackMelee(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup   = ELeeAbilityActivationGroup::Exclusive_Blocking;

	// 어빌리티가 활성 동안 ASC에 자동 부여될 태그 (EndAbility 시 GAS가 자동 제거)
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Attack_Attacking);

	// TryActivateAbilitiesByTag 호출 시 이 태그로 매칭됨
	AbilityTags.AddTag(MyTags::Souls::Status_Attack_Melee);

	// ANS_ToggleTrace와의 약속 — 기존 코드 그대로 재사용
	TraceEventTag = MyTags::Abilities::Enemy::Trace;
}

void ULeeGameplayAbility_AttackMelee::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 아바타 유효성 검사
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 스태미나 소모, 쿨다운 적용 등 CommitAbility 처리
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 랜덤 몽타주 선택
	if (AttackMontages.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_AttackMelee] AttackMontages 배열이 비어있음. BP에서 몽타주를 등록해주세요."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SelectedMontage = AttackMontages[FMath::RandRange(0, AttackMontages.Num() - 1)];
	if (!SelectedMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_AttackMelee] 선택된 몽타주가 null입니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. PlayMontageAndWait 태스크 실행
	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, SelectedMontage, /*PlayRate*/1.0f);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();

	// 3. ANS_ToggleTrace로부터 HitResult 이벤트 대기
	//    OnlyTriggerOnce=false: 한 공격 모션 내에서 여러 타겟 히트 허용
	if (TraceEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* EventTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this,
				TraceEventTag,
				/*OptionalExternalOwner*/nullptr,
				/*OnlyTriggerOnce*/false,
				/*OnlyMatchExact*/true);

		EventTask->EventReceived.AddDynamic(this, &ThisClass::OnTraceEventReceived);
		EventTask->ReadyForActivation();
	}
}

void ULeeGameplayAbility_AttackMelee::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// ActivationOwnedTags의 Status_Attack_Attacking은 GAS가 여기서 자동 제거
	// StateTree Task가 이 태그 제거를 감지하여 FinishTask를 호출함
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULeeGameplayAbility_AttackMelee::OnTraceEventReceived(FGameplayEventData Payload)
{
	// Payload.Instigator: ANS_ToggleTrace가 설정하는 공격자 (GetOwner() 아님)
	AActor* AttackerActor = const_cast<AActor*>(Payload.Instigator.Get());
	if (!AttackerActor)
	{
		return;
	}

	// TargetData에 HitResult가 없으면 처리하지 않음
	if (!UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(Payload.TargetData, 0))
	{
		return;
	}

	const FHitResult HitResult =
		UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(Payload.TargetData, 0);

	AActor* HitActor = HitResult.GetActor();
	if (!HitActor || HitActor == AttackerActor)
	{
		return;
	}

	// 데미지 GE가 설정되지 않았으면 처리 생략
	if (!DamageEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_AttackMelee] DamageEffect가 설정되지 않음. BP에서 GE_MeleeDamage를 지정해주세요."));
		return;
	}

	UAbilitySystemComponent* AttackerASC = GetActorInfo().AbilitySystemComponent.Get();
	UAbilitySystemComponent* HitASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);

	if (!AttackerASC || !HitASC)
	{
		return;
	}

	// SetByCaller 방식으로 데미지 GE 적용
	FGameplayEffectContextHandle Context = AttackerASC->MakeEffectContext();
	Context.AddHitResult(HitResult);

	FGameplayEffectSpecHandle SpecHandle =
		AttackerASC->MakeOutgoingSpec(DamageEffect, /*Level*/1.0f, Context);

	if (SpecHandle.IsValid())
	{
		// 음수로 설정하여 Health 감소 (AttributeSet의 PostGameplayEffectExecute에서 처리)
		SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Damage, -DamageAmount);
		HitASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ULeeGameplayAbility_AttackMelee::OnMontageCompleted()
{
	// 정상 완료: 다음 공격 가능 상태로 복귀
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}

void ULeeGameplayAbility_AttackMelee::OnMontageInterrupted()
{
	// 외부 취소 (피격 그로기, 처형 등으로 인한 강제 종료)
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/true);
}
