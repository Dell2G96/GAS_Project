// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbility_BowFire.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AWeapons/LeeArrowProjectile.h"

// 좌클릭 발사 어빌리티 식별 태그
ULeeGameplayAbility_BowFire::ULeeGameplayAbility_BowFire(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(MyTags::Souls::Ability_Type_Action_BowFire);
	SetAssetTags(AssetTags);
}

// 조준 중 → 발사 요청만 보내고 종료 / 조준 아님 → 속사 (쿨다운 적용 후 활을 최소한으로 당기고 약한 화살 발사)
void ULeeGameplayAbility_BowFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!ASC || !Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 조준 중: 클라·서버 각자 자기 쪽 조준 어빌리티에 알린다 (서버 쪽이 실제 화살을 쏜다)
	if (ASC->HasMatchingGameplayTag(MyTags::Souls::Ability_Type_Action_BowAim))
	{
		FGameplayEventData Payload;
		Payload.Instigator = Avatar;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar, MyTags::Souls::GameplayEvent_Bow_ReleaseShot, Payload);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 속사
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 최소 시위: 활 시위를 당긴 상태로 만들고 서버는 손에 화살을 건다
	bQuickShotReleased = false;
	HeldArrow = nullptr;
	SetBowStretched(true);

	// 화살은 카메라 방향으로 날아가므로 캐릭터도 카메라 방향으로 돌린다 (이동 방향을 보면 뒤로 걸으며 쏠 때 정반대가 됨)
	SetFaceCameraRotation(true);
	if (IsServerSide())
	{
		HeldArrow = SpawnArrowInHand();
	}

	if (QuickDrawMontage)
	{
		// 몽타주는 모습만 담당한다. Loop 섹션이 있으면 스스로 끝나지 않으므로 끝 콜백에 의존하지 않고 시간으로 놓는다
		UAbilityTask_PlayMontageAndWait* DrawTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, QuickDrawMontage);
		DrawTask->OnInterrupted.AddDynamic(this, &ThisClass::OnQuickDrawCancelled);
		DrawTask->OnCancelled.AddDynamic(this, &ThisClass::OnQuickDrawCancelled);
		DrawTask->ReadyForActivation();

		UAbilityTask_WaitDelay* DrawDelay = UAbilityTask_WaitDelay::WaitDelay(this, QuickDrawTime);
		DrawDelay->OnFinish.AddDynamic(this, &ThisClass::OnQuickDrawFinished);
		DrawDelay->ReadyForActivation();
	}
	else
	{
		// 당기기 몽타주가 지정되지 않았으면 당기지 않고 바로 쏜다
		ReleaseQuickShot();
	}
}

// 종료 — 쏘지 못한 손 화살 제거, 활 시위 원위치, 회전 방식 복구
void ULeeGameplayAbility_BowFire::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	if (HeldArrow && !HeldArrow->IsLaunched())
	{
		HeldArrow->Destroy();
	}
	HeldArrow = nullptr;

	// 조준 어빌리티가 활을 당기고 있는 중이면 그쪽이 관리하므로 건드리지 않는다
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !ASC->HasMatchingGameplayTag(MyTags::Souls::Ability_Type_Action_BowAim))
	{
		SetBowStretched(false);
	}

	// 속사가 바꿔 둔 회전 방식만 되돌린다 (조준 경유로 끝난 경우엔 바꾼 적이 없어 아무 일도 안 함)
	SetFaceCameraRotation(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// 당기는 시간(QuickDrawTime)이 지남 — 화살 발사
void ULeeGameplayAbility_BowFire::OnQuickDrawFinished()
{
	ReleaseQuickShot();
}

// 당기는 도중 다른 동작에 끊김 — 쏘지 않고 종료
void ULeeGameplayAbility_BowFire::OnQuickDrawCancelled()
{
	if (!bQuickShotReleased)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

// 시위를 놓는다: 활 원위치 → 서버가 화살 발사 → 반동 몽타주가 끝나면 종료. 중복 호출은 무시
void ULeeGameplayAbility_BowFire::ReleaseQuickShot()
{
	if (bQuickShotReleased)
	{
		return;
	}
	bQuickShotReleased = true;

	SetBowStretched(false);

	if (IsServerSide())
	{
		ALeeArrowProjectile* Arrow = HeldArrow ? HeldArrow.Get() : SpawnArrowInHand();
		HeldArrow = nullptr;
		LaunchArrow(Arrow, QuickShot);
	}

	if (FireMontage)
	{
		UAbilityTask_PlayMontageAndWait* FireTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, FireMontage);
		FireTask->OnCompleted.AddDynamic(this, &ThisClass::OnFireMontageEnded);
		FireTask->OnBlendOut.AddDynamic(this, &ThisClass::OnFireMontageEnded);
		FireTask->OnInterrupted.AddDynamic(this, &ThisClass::OnFireMontageEnded);
		FireTask->OnCancelled.AddDynamic(this, &ThisClass::OnFireMontageEnded);
		FireTask->ReadyForActivation();
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

// 속사 몽타주가 끝나면 어빌리티 종료
void ULeeGameplayAbility_BowFire::OnFireMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
