// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbility_BowAim.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "TimerManager.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACamera/LeeCameraShake_Bow.h"
#include "GAS_Project/ACharacter/LeeTargetLockComponent.h"
#include "GAS_Project/AWeapons/LeeArrowProjectile.h"

// 조준 중 태그(애님·퀵슬롯 차단용)와 기본 카메라 쉐이크 지정
ULeeGameplayAbility_BowAim::ULeeGameplayAbility_BowAim(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(MyTags::Souls::Ability_Type_Action_BowAim);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(MyTags::Souls::Ability_Type_Action_BowAim);
	ActivationOwnedTags.AddTag(MyTags::Souls::Event_Movement_BowAnim);

	ChargeShakeClass = ULeeCameraShake_BowCharge::StaticClass();
	FullChargeShakeClass = ULeeCameraShake_BowFullCharge::StaticClass();
}

// 우클릭 → 활 당기기 시작. 다 당김·발사 요청·우클릭 뗌을 각각 기다린다
void ULeeGameplayAbility_BowAim::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	Phase = EAimPhase::Drawing;
	ChargeStartTime = 0.0f;
	HeldArrow = nullptr;

	SetAimRotation(true);

	// 몽타주 안의 SpawnArrow 노티파이 = 다 당김
	UAbilityTask_WaitGameplayEvent* WaitDrawn = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, MyTags::Souls::GameplayEvent_Bow_SpawnArrow, nullptr, /*OnlyTriggerOnce*/ true);
	WaitDrawn->EventReceived.AddDynamic(this, &ThisClass::OnSpawnArrowEvent);
	WaitDrawn->ReadyForActivation();

	// 조준 중 좌클릭 발사 요청 (GA_BowFire가 보냄)
	UAbilityTask_WaitGameplayEvent* WaitShot = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, MyTags::Souls::GameplayEvent_Bow_ReleaseShot, nullptr, /*OnlyTriggerOnce*/ false);
	WaitShot->EventReceived.AddDynamic(this, &ThisClass::OnReleaseShotEvent);
	WaitShot->ReadyForActivation();

	// 우클릭 뗌 → 취소(쏘기 전) 또는 종료(쏜 뒤)
	UAbilityTask_WaitInputRelease* WaitRelease = UAbilityTask_WaitInputRelease::WaitInputRelease(this, /*bTestAlreadyReleased*/ true);
	WaitRelease->OnRelease.AddDynamic(this, &ThisClass::OnAimInputReleased);
	WaitRelease->ReadyForActivation();

	UE_LOG(LogTemp, Log, TEXT("[Bow] 조준 시작 DrawMontage=%s Server=%d Local=%d"), *GetNameSafe(DrawMontage), IsServerSide(), IsLocalSide());

	if (DrawMontage)
	{
		// 노티파이가 없는 몽타주여도 몽타주가 끝나면 다 당긴 것으로 본다
		UAbilityTask_PlayMontageAndWait* DrawTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, DrawMontage);
		DrawTask->OnCompleted.AddDynamic(this, &ThisClass::OnDrawMontageFinished);
		DrawTask->OnBlendOut.AddDynamic(this, &ThisClass::OnDrawMontageFinished);
		DrawTask->ReadyForActivation();
	}
	else
	{
		StartCharging();
	}
}

// 조준 종료 — 타이머·흔들림·UI 정리, 쏘지 않은 손 화살 제거, 회전 방식 복구
void ULeeGameplayAbility_BowAim::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MaxChargeTimerHandle);
	}
	StopChargeShake();

	if (Phase == EAimPhase::Charging)
	{
		BroadcastCharge(false);
	}

	SetBowStretched(false);

	if (IsServerSide() && HeldArrow && !HeldArrow->IsLaunched())
	{
		HeldArrow->Destroy();
	}
	HeldArrow = nullptr;

	SetAimRotation(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// 당기기 몽타주가 끝남 — 노티파이를 못 받았어도 차지 시작
void ULeeGameplayAbility_BowAim::OnDrawMontageFinished()
{
	StartCharging();
}

// SpawnArrow 노티파이 — 다 당김
void ULeeGameplayAbility_BowAim::OnSpawnArrowEvent(FGameplayEventData Payload)
{
	StartCharging();
}

// 조준 중 좌클릭 — 지금 차지만큼 발사 (다 당기기 전이면 약한 화살)
void ULeeGameplayAbility_BowAim::OnReleaseShotEvent(FGameplayEventData Payload)
{
	FireShot(/*bFullCharge*/ false);
}

// 우클릭 뗌 — 쏘기 전이면 취소, 쏜 뒤면 정상 종료
void ULeeGameplayAbility_BowAim::OnAimInputReleased(float TimeHeld)
{
	const bool bCancelled = (Phase != EAimPhase::Fired);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, bCancelled);
}

// 다 당김: 서버는 손에 화살을 붙이고, 양쪽 모두 최대 차지 타이머 시작, 로컬은 흔들림·UI 시작
void ULeeGameplayAbility_BowAim::StartCharging()
{
	if (Phase != EAimPhase::Drawing)
	{
		return;
	}

	Phase = EAimPhase::Charging;
	ChargeStartTime = GetWorld()->GetTimeSeconds();

	SetBowStretched(true);

	if (IsServerSide() && !HeldArrow)
	{
		HeldArrow = SpawnArrowInHand();
	}
	UE_LOG(LogTemp, Log, TEXT("[Bow] 차지 시작 Server=%d Local=%d HeldArrow=%s"), IsServerSide(), IsLocalSide(), *GetNameSafe(HeldArrow));

	GetWorld()->GetTimerManager().SetTimer(MaxChargeTimerHandle,
		FTimerDelegate::CreateUObject(this, &ThisClass::OnMaxChargeReached), MaxChargeTime, false);

	StartChargeShake();
	BroadcastCharge(true);
}

// 최대 차지 — 큰 흔들림과 함께 자동 발사
void ULeeGameplayAbility_BowAim::OnMaxChargeReached()
{
	FireShot(/*bFullCharge*/ true);
}

// 발사: 쿨다운 적용(Commit) → 반동 몽타주 → 서버가 차지 비율로 데미지·속도·사거리를 정해 화살을 날린다
void ULeeGameplayAbility_BowAim::FireShot(bool bFullCharge)
{
	if (Phase == EAimPhase::Fired)
	{
		return;
	}

	const bool bWasCharging = (Phase == EAimPhase::Charging);
	const float Ratio = bFullCharge ? 1.0f : GetChargeRatio();

	// 쿨다운 중이면 쏘지 않는다 (조준은 유지)
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		return;
	}

	Phase = EAimPhase::Fired;

	GetWorld()->GetTimerManager().ClearTimer(MaxChargeTimerHandle);
	StopChargeShake();
	if (bWasCharging)
	{
		BroadcastCharge(false);
	}

	if (bFullCharge && IsLocalSide())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetControllerFromActorInfo()))
		{
			if (PC->PlayerCameraManager && FullChargeShakeClass)
			{
				PC->PlayerCameraManager->StartCameraShake(FullChargeShakeClass, FullChargeShakeScale);
			}
		}
	}

	SetBowStretched(false);

	if (FireMontage)
	{
		UAbilityTask_PlayMontageAndWait* FireTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, FireMontage);
		FireTask->ReadyForActivation();
	}

	if (IsServerSide())
	{
		FLeeBowShotParams Shot = WeakShot;
		if (bWasCharging)
		{
			Shot.Damage = FMath::Lerp(MinChargeShot.Damage, MaxChargeShot.Damage, Ratio);
			Shot.Speed = FMath::Lerp(MinChargeShot.Speed, MaxChargeShot.Speed, Ratio);
			Shot.Range = FMath::Lerp(MinChargeShot.Range, MaxChargeShot.Range, Ratio);
		}

		ALeeArrowProjectile* Arrow = HeldArrow ? HeldArrow.Get() : SpawnArrowInHand();
		HeldArrow = nullptr;
		LaunchArrow(Arrow, Shot);

		UE_LOG(LogTemp, Log, TEXT("[Bow] Fire charged=%d ratio=%.2f full=%d"), bWasCharging, bWasCharging ? Ratio : 0.0f, bFullCharge);
	}
}

// 차지 비율 = 다 당긴 뒤 지난 시간 / 최대 차지 시간 (0~1, 직선)
float ULeeGameplayAbility_BowAim::GetChargeRatio() const
{
	if (Phase != EAimPhase::Charging)
	{
		return 0.0f;
	}
	const float Elapsed = GetWorld()->GetTimeSeconds() - ChargeStartTime;
	return FMath::Clamp(Elapsed / FMath::Max(MaxChargeTime, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
}

// 조준 중에는 이동 방향이 아니라 카메라 방향을 보게 한다.
// 락온 중에는 락온 컴포넌트가 회전 방식의 주인이므로 건드리지 않는다.
// 조준 중에 락온이 시작돼 종료 때까지 이어지면, 락온이 나중에 복구할 "원래 값"만 되돌려 준다(안 하면 락온 해제 후에도 회전이 안 돌아옴)
void ULeeGameplayAbility_BowAim::SetAimRotation(bool bAiming)
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UCharacterMovementComponent* MoveComp = Character ? Character->GetCharacterMovement() : nullptr;
	if (!MoveComp)
	{
		return;
	}

	ULeeTargetLockComponent* Lock = ULeeTargetLockComponent::FindTargetLockComponent(Character);
	const bool bLocked = Lock && Lock->IsLocked();

	if (bAiming)
	{
		if (!bLocked && MoveComp->bOrientRotationToMovement)
		{
			bOriginalOrientRotation = MoveComp->bOrientRotationToMovement;
			MoveComp->bOrientRotationToMovement = false;
			bChangedOrientRotation = true;
		}
	}
	else if (bChangedOrientRotation)
	{
		bChangedOrientRotation = false;

		if (bLocked)
		{
			// 락온 중: 플래그는 락온 것(false 유지). 락온 해제 때 복구할 값만 원래 값으로 정정
			Lock->SetSavedOrientRotationToMovement(bOriginalOrientRotation);
		}
		else
		{
			MoveComp->bOrientRotationToMovement = bOriginalOrientRotation;
		}
	}
}

// [로컬] 차지 흔들림 시작 — 세기는 0에서 시작해 차지 비율에 따라 선형으로 커진다
void ULeeGameplayAbility_BowAim::StartChargeShake()
{
	if (!IsLocalSide() || !ChargeShakeClass)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetControllerFromActorInfo());
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}

	ChargeShakeInstance = PC->PlayerCameraManager->StartCameraShake(ChargeShakeClass, 0.0f);
	GetWorld()->GetTimerManager().SetTimer(ShakeUpdateTimerHandle,
		FTimerDelegate::CreateUObject(this, &ThisClass::UpdateChargeShake), 0.05f, true);
}

// [로컬] 흔들림 세기 = 차지 비율 × 최대 세기
void ULeeGameplayAbility_BowAim::UpdateChargeShake()
{
	if (ChargeShakeInstance)
	{
		ChargeShakeInstance->ShakeScale = GetChargeRatio() * ChargeShakeMaxScale;
	}
}

// [로컬] 차지 흔들림 정지
void ULeeGameplayAbility_BowAim::StopChargeShake()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShakeUpdateTimerHandle);
	}

	if (ChargeShakeInstance)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetControllerFromActorInfo()))
		{
			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StopCameraShake(ChargeShakeInstance, /*bImmediately*/ false);
			}
		}
		ChargeShakeInstance = nullptr;
	}
}

// [로컬] 차지 게이지 위젯에 시작/끝 알림
void ULeeGameplayAbility_BowAim::BroadcastCharge(bool bCharging) const
{
	if (!IsLocalSide())
	{
		return;
	}

	FLeeBowChargeMessage Message;
	Message.Instigator = GetAvatarActorFromActorInfo();
	Message.bCharging = bCharging;
	Message.ChargeStartTime = ChargeStartTime;
	Message.MaxChargeTime = MaxChargeTime;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(MyTags::Souls::Message_Bow_Charge, Message);
}
