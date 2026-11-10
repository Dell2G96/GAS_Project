// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbility_BowBase.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS_Project/ACharacter/LeeTargetLockComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AWeapons/LeeArrowProjectile.h"

// 활 어빌리티 공통 설정 — 액터별 인스턴스, 클라 예측 실행, 행동 중에는 시작 불가
ULeeGameplayAbility_BowBase::ULeeGameplayAbility_BowBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Attack_Attacking);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Guard_Active);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Dodge_Active);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Estus_Drinking);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Groggy);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Executing);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Assassinating);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Finisher_Victim);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Downed);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Death_Dying);
	ActivationBlockedTags.AddTag(MyTags::Souls::Status_Death_Dead);
}

// 서버에서 화살을 스폰해 캐릭터 손 소켓에 붙인다
ALeeArrowProjectile* ULeeGameplayAbility_BowBase::SpawnArrowInHand() const
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsServerSide() || !Character || !ArrowClass)
	{
		// 조용히 실패하면 "화살이 안 나간다"로만 보이므로 이유를 남긴다
		UE_LOG(LogTemp, Warning, TEXT("[Bow] 화살 스폰 불가: Server=%d Character=%s ArrowClass=%s"),
			IsServerSide(), *GetNameSafe(Character), *GetNameSafe(ArrowClass));
		return nullptr;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	const bool bHasSocket = Mesh && Mesh->DoesSocketExist(ArrowSocketName);
	const FTransform SpawnTransform = bHasSocket ? Mesh->GetSocketTransform(ArrowSocketName) : Character->GetActorTransform();

	ALeeArrowProjectile* Arrow = GetWorld()->SpawnActorDeferred<ALeeArrowProjectile>(
		ArrowClass, SpawnTransform, Character, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Arrow)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bow] 화살 스폰 실패(SpawnActorDeferred null): Class=%s Transform=%s"),
			*GetNameSafe(ArrowClass), *SpawnTransform.ToString());
		return nullptr;
	}
	UE_LOG(LogTemp, Log, TEXT("[Bow] 화살 스폰 Arrow=%s Socket=%s(%d)"), *GetNameSafe(Arrow), *ArrowSocketName.ToString(), bHasSocket);

	// 발사자를 먼저 알려 둬야 BeginPlay에서 자기 몸과 부딪히지 않게 설정된다
	Arrow->InitProjectile(FGameplayEffectSpecHandle(), Character);
	Arrow->FinishSpawning(SpawnTransform);

	if (Mesh && Mesh->DoesSocketExist(ArrowSocketName))
	{
		Arrow->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ArrowSocketName);
	}
	return Arrow;
}

// 서버에서 데미지 Spec(SetByCaller)을 만들어 화살에 싣고 조준 방향으로 날린다
void ULeeGameplayAbility_BowBase::LaunchArrow(ALeeArrowProjectile* Arrow, const FLeeBowShotParams& Shot) const
{
	if (!IsServerSide() || !Arrow)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bow] 발사 불가: Server=%d Arrow=%s"), IsServerSide(), *GetNameSafe(Arrow));
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!ASC || !Avatar)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bow] 발사 불가: ASC=%s Avatar=%s"), *GetNameSafe(ASC), *GetNameSafe(Avatar));
		Arrow->Destroy();
		return;
	}

	FGameplayEffectSpecHandle SpecHandle;
	if (DamageEffect)
	{
		SpecHandle = ASC->MakeOutgoingSpec(DamageEffect, GetAbilityLevel(), ASC->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Damage, -Shot.Damage);
			SpecHandle.Data->AddDynamicAssetTag(MyTags::Souls::DamageType_Attack_Projectile);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bow] %s: DamageEffect가 비어 있어 데미지 없이 발사합니다. BP에서 지정하세요."), *GetName());
	}

	Arrow->InitProjectile(SpecHandle, Avatar);
	Arrow->Launch(GetAimDirection(Arrow->GetActorLocation(), Shot.Range), Shot.Speed, Shot.Range);

	UE_LOG(LogTemp, Log, TEXT("[Bow] Launch damage=%.1f speed=%.0f range=%.0f"), Shot.Damage, Shot.Speed, Shot.Range);
}

// 카메라 중앙이 가리키는 지점(벽·적)을 찾아 그 지점을 향하는 방향을 돌려준다
FVector ULeeGameplayAbility_BowBase::GetAimDirection(const FVector& From, float Range) const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	const APlayerController* PC = Cast<APlayerController>(GetControllerFromActorInfo());
	if (!PC || !Avatar)
	{
		return Avatar ? Avatar->GetActorForwardVector() : FVector::ForwardVector;
	}

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector TraceEnd = CamLoc + CamRot.Vector() * (Range + FVector::Dist(CamLoc, From));
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BowAim), false, Avatar);

	FHitResult Hit;
	const FVector AimPoint = GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_Visibility, Params)
		? Hit.ImpactPoint
		: TraceEnd;

	const FVector Dir = (AimPoint - From).GetSafeNormal();
	return Dir.IsNearlyZero() ? CamRot.Vector() : Dir;
}

// 캐릭터에 붙은 액터 중 BowStretchPropertyName bool 변수가 있는 것(B_Bow)의 값을 바꾼다
void ULeeGameplayAbility_BowBase::SetBowStretched(bool bStretched) const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || BowStretchPropertyName.IsNone())
	{
		return;
	}

	TArray<AActor*> AttachedActors;
	Avatar->GetAttachedActors(AttachedActors);
	for (AActor* Attached : AttachedActors)
	{
		if (const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Attached->GetClass(), BowStretchPropertyName))
		{
			Prop->SetPropertyValue_InContainer(Attached, bStretched);
		}
	}
}

// 서버(권위)에서 실행 중인지
bool ULeeGameplayAbility_BowBase::IsServerSide() const
{
	return CurrentActorInfo && CurrentActorInfo->IsNetAuthority();
}

// 조작하는 플레이어 쪽(로컬)에서 실행 중인지
bool ULeeGameplayAbility_BowBase::IsLocalSide() const
{
	return CurrentActorInfo && CurrentActorInfo->IsLocallyControlled();
}

// 쏘는 동안 이동 방향이 아니라 카메라 방향을 보게 한다 (bUseControllerDesiredRotation이 켜져 있어 RotationRate 속도로 돈다).
// 락온 중에는 락온 컴포넌트가 회전 방식의 주인이므로 건드리지 않는다.
// 쏘는 도중 락온이 시작돼 종료 때까지 이어지면, 락온이 나중에 복구할 "원래 값"만 되돌려 준다(안 하면 락온 해제 후에도 회전이 안 돌아옴)
void ULeeGameplayAbility_BowBase::SetFaceCameraRotation(bool bFaceCamera)
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UCharacterMovementComponent* MoveComp = Character ? Character->GetCharacterMovement() : nullptr;
	if (!MoveComp)
	{
		return;
	}

	ULeeTargetLockComponent* Lock = ULeeTargetLockComponent::FindTargetLockComponent(Character);
	const bool bLocked = Lock && Lock->IsLocked();

	if (bFaceCamera)
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
