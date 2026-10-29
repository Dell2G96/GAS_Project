// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbility_EnemyRangedAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AIController.h"
#include "MotionWarpingComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AWeapons/LeeProjectile.h"

ULeeGameplayAbility_EnemyRangedAttack::ULeeGameplayAbility_EnemyRangedAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup    = ELeeAbilityActivationGroup::Exclusive_Blocking;

	// 지금은 원거리 어빌리티가 하나뿐이라(Light/Heavy 구분 없음) 식별 태그를 생성자에 고정한다.
	AbilityTags.AddTag(MyTags::Souls::Ability_Attack_Ranged);

	// StateTree의 Lee Activate Ability Task가 태그 폴링 fallback으로 종료를 판정하는 경로이므로 필수.
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Attack_Attacking);

	FireEventTag = MyTags::Souls::Event_Attack_Fire;
}

void ULeeGameplayAbility_EnemyRangedAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AttackDataList.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_EnemyRangedAttack] AttackDataList가 비어있습니다. BP에서 공격 데이터를 확인해주세요."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FLeeRangedAttackData& SelectedAttackData = AttackDataList[FMath::RandRange(0, AttackDataList.Num() - 1)];
	if (!SelectedAttackData.ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_EnemyRangedAttack] 선택된 공격 데이터의 투사체 클래스가 비어있습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	CurrentAttackData = SelectedAttackData;

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UpdateFacingWarpTarget();

	// [임시/테스트용] 2026.10.19 — Morgana용 캐스팅 몽타주가 아직 없어 Montage가 비면
	// Fire 이벤트/몽타주 재생 없이 즉시 발사하고 바로 종료한다. 몽타주가 준비되면 이 분기를 지운다.
	if (!CurrentAttackData.Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_EnemyRangedAttack] Montage가 비어있어 애니메이션 없이 즉시 발사합니다 (임시)."));
		FireOnce();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Fire 이벤트 대기 — 다연발 몽타주를 지원하기 위해 반복 수신(OnlyTriggerOnce=false)
	if (FireEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* FireTask =
			UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, FireEventTag, /*OptionalExternalOwner*/nullptr, /*OnlyTriggerOnce*/false, /*OnlyMatchExact*/true);

		FireTask->EventReceived.AddDynamic(this, &ThisClass::OnFireEventReceived);
		FireTask->ReadyForActivation();
	}

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, CurrentAttackData.Montage, /*PlayRate*/1.0f);

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
}

// 조준 기준 대상 — AttackMelee::GetWarpFacingTarget과 동일 규약: AIController의 네이티브 Focus를 그대로 읽는다
AActor* ULeeGameplayAbility_EnemyRangedAttack::GetAimTarget() const
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AAIController* AIC = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
	return AIC ? AIC->GetFocusActor() : nullptr;
}

// 회전 전용 워프 타깃 갱신 (AttackMelee의 "방식 B"와 동일 — 위치는 그대로, 방향만 대상 쪽으로)
void ULeeGameplayAbility_EnemyRangedAttack::UpdateFacingWarpTarget()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UMotionWarpingComponent* MW = Avatar ? Avatar->FindComponentByClass<UMotionWarpingComponent>() : nullptr;
	if (!MW)
	{
		return;
	}

	AActor* Target = GetAimTarget();
	if (!Target)
	{
		MW->RemoveWarpTarget(WarpTargetName);
		return;
	}

	const FVector AvatarLoc = Avatar->GetActorLocation();
	const FVector ToTargetDir = (Target->GetActorLocation() - AvatarLoc).GetSafeNormal2D();
	MW->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, AvatarLoc, ToTargetDir.Rotation());
}

// Fire 이벤트 수신 — 그 시점 타겟의 현재 위치로 투사체 1개를 발사한다(선행 조준 없음)
void ULeeGameplayAbility_EnemyRangedAttack::OnFireEventReceived(FGameplayEventData Payload)
{
	if (!GetActorInfo().IsNetAuthority() || !IsActive())
	{
		return;
	}

	FireOnce();
}

// 실제 발사 처리 — 그 시점 타겟의 현재 위치로 투사체 1개를 발사한다(선행 조준 없음)
void ULeeGameplayAbility_EnemyRangedAttack::FireOnce()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !CurrentAttackData.ProjectileClass)
	{
		return;
	}

	AActor* Target = GetAimTarget();
	if (!Target)
	{
		// 발사 순간 타겟이 없으면 유령 발사를 막는다 (사망/타겟 이탈 직후 등)
		return;
	}

	const ACharacter* Character = Cast<ACharacter>(Avatar);
	const FVector MuzzleLocation =
		(Character && Character->GetMesh() && Character->GetMesh()->DoesSocketExist(CurrentAttackData.MuzzleSocketName))
		? Character->GetMesh()->GetSocketLocation(CurrentAttackData.MuzzleSocketName)
		: Avatar->GetActorLocation();

	const FVector AimDirection = (Target->GetActorLocation() - MuzzleLocation).GetSafeNormal();
	const FRotator AimRotation = AimDirection.Rotation();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !DamageEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_EnemyRangedAttack] DamageEffect가 설정되지 않음. BP에서 지정해주세요."));
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DamageEffect, GetAbilityLevel(), ASC->MakeEffectContext());
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Damage, -CurrentAttackData.BaseDamage);
	SpecHandle.Data->AddDynamicAssetTag(MyTags::Souls::DamageType_Attack_Projectile);
	for (const FGameplayTag& DamageTypeTag : CurrentAttackData.DamageTypeTags)
	{
		SpecHandle.Data->AddDynamicAssetTag(DamageTypeTag);
	}

	UWorld* World = Avatar->GetWorld();
	ALeeProjectile* Projectile = World->SpawnActorDeferred<ALeeProjectile>(
		CurrentAttackData.ProjectileClass, FTransform(AimRotation, MuzzleLocation),
		Avatar, Cast<APawn>(Avatar), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (Projectile)
	{
		Projectile->InitProjectile(SpecHandle, Avatar);
		Projectile->InitKnockback(CurrentAttackData.KnockbackOffset, CurrentAttackData.KnockbackDuration, CurrentAttackData.GuardKnockbackScale);
		Projectile->FinishSpawning(FTransform(AimRotation, MuzzleLocation));
	}

	UE_LOG(LogTemp, Log, TEXT("[LeeRanged] Fire actor=%s target=%s"), *GetNameSafe(Avatar), *GetNameSafe(Target));
}

void ULeeGameplayAbility_EnemyRangedAttack::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}

void ULeeGameplayAbility_EnemyRangedAttack::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/true);
}

void ULeeGameplayAbility_EnemyRangedAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	if (AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr)
	{
		if (UMotionWarpingComponent* MW = Avatar->FindComponentByClass<UMotionWarpingComponent>())
		{
			MW->RemoveWarpTarget(WarpTargetName);
		}
	}

	CurrentAttackData = FLeeRangedAttackData();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
