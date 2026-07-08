// Fill out your copyright notice in the Description page of Project Settings.

#include "LeeGameplayAbility_Finisher.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "MotionWarpingComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/ACamera/LeeCameraMode.h"
#include "GAS_Project/ACharacter/LeeHeroComponent.h"
#include "GAS_Project/AEquipment/LeeMeleeWeaponInstance.h"

ULeeGameplayAbility_Finisher::ULeeGameplayAbility_Finisher(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup    = ELeeAbilityActivationGroup::Exclusive_Blocking;

	// 처형/암살 공용 어빌리티 — 두 식별 태그를 모두 부착
	AbilityTags.AddTag(MyTags::Souls::Ability_Execution);
	AbilityTags.AddTag(MyTags::Souls::Ability_Assassination);

	// 시퀀스 동안 공격자 이동/회전 완전 정지 (LeeCharacterMovementComponent가 이 태그를 감지해 MaxSpeed=0 처리)
	ActivationOwnedTags.AddTag(MyTags::Souls::Gameplay_MovementStopped);
}

bool ULeeGameplayAbility_Finisher::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 조건을 만족하는 타겟이 실행 거리 내에 있어야만 활성화 가능
	ELeeFinisherType UnusedType;
	return FindFinisherTarget(ActorInfo, UnusedType) != nullptr;
}

void ULeeGameplayAbility_Finisher::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 서버에서 타겟 재검증 (분기 지점 ① — CurrentType 확정)
	AActor* Target = FindFinisherTarget(ActorInfo, CurrentType);
	if (!Target)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	CurrentTarget = Target;

	// 2. 무기별 FinisherData 조회 (분기 지점 ② — AnimSet 선택)
	const ULeeMeleeWeaponInstance* Weapon = Cast<ULeeMeleeWeaponInstance>(GetAssociatedEquipment());
	const ULeeFinisherData* FinisherData = Weapon ? Weapon->GetFinisherData() : nullptr;
	if (!FinisherData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_Finisher] FinisherData가 없습니다. 무기 EquipmentDefinition의 InstanceType이 LeeMeleeWeaponInstance인지, FinisherData 에셋이 지정되었는지 확인해주세요."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentAnimSet = FinisherData->GetAnimSet(CurrentType);
	if (!CurrentAnimSet.AttackerMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_Finisher] %s의 %s AttackerMontage가 비어있습니다."),
			*FinisherData->GetName(),
			(CurrentType == ELeeFinisherType::Execution) ? TEXT("Execution") : TEXT("Assassination"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	// 3. 무적 적용 — 핸들을 보관하고 EndAbility에서 반드시 제거한다
	if (InvincibleEffect)
	{
		const FGameplayEffectSpecHandle InvincibleSpec =
			MakeOutgoingGameplayEffectSpec(InvincibleEffect, GetAbilityLevel());
		InvincibleHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, InvincibleSpec);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_Finisher] InvincibleEffect가 설정되지 않음. BP에서 GE_FinisherInvincible을 지정해주세요."));
	}

	// 4. 진행 상태 태그 부여 (Status.Executing / Status.Assassinating) — EndAbility에서 제거
	AppliedStatusTag = (CurrentType == ELeeFinisherType::Execution)
		? MyTags::Souls::Status_Executing
		: MyTags::Souls::Status_Assassinating;
	if (ASC)
	{
		ASC->AddLooseGameplayTag(AppliedStatusTag);
	}

	// 5. 공격자/피해자 캡슐 상호 무시 — EndAbility에서 반드시 원복
	//    워프 접근을 캡슐 충돌이 막으면 목표 지점에 캡슐 반경만큼 못 미쳐 정렬이 어긋난다 (정렬 오차 1순위 원인).
	//    전역 콜리전 비활성은 쓰지 않는다 — 바닥/벽 충돌은 유지되어야 함.
	//    서버/소유 클라이언트 양쪽에서 실행됨 (공격자 이동은 클라이언트 예측이므로 클라이언트에도 필요)
	if (ACharacter* AvatarCharacter = Cast<ACharacter>(Avatar))
	{
		AvatarCharacter->MoveIgnoreActorAdd(Target);
	}
	if (ACharacter* TargetCharacter = Cast<ACharacter>(Target))
	{
		TargetCharacter->MoveIgnoreActorAdd(Avatar);
	}

	// 6. Motion Warping 타겟 설정 — 피해자의 최종 회전 기준 AttackerOffset 위치로 정렬
	const FVector AttackerLocation = Avatar->GetActorLocation();
	const FVector VictimLocation = Target->GetActorLocation();
	const float VictimYaw = ComputeVictimYaw(CurrentType, AttackerLocation, VictimLocation);
	const FTransform VictimTransform(FRotator(0.0f, VictimYaw, 0.0f), VictimLocation);
	const FTransform WarpTransform = CurrentAnimSet.AttackerOffset * VictimTransform;

	// Z는 워프하지 않는다 — 두 캐릭터 캡슐 높이가 다르면 공격자가 뜨거나 파묻히므로 중력/바닥 충돌에 맡긴다
	FVector WarpLocation = WarpTransform.GetLocation();
	WarpLocation.Z = AttackerLocation.Z;

	if (CurrentAnimSet.bSnapAttackerAtStart)
	{
		// 첫 프레임부터 완전 정렬이 필요한 연출: 워프 수렴 대신 즉시 스냅
		Avatar->SetActorLocationAndRotation(WarpLocation, WarpTransform.Rotator(),
			/*bSweep*/false, /*OutHit*/nullptr, ETeleportType::TeleportPhysics);
	}
	else if (UMotionWarpingComponent* MotionWarping = Avatar->FindComponentByClass<UMotionWarpingComponent>())
	{
		// 몽타주의 Motion Warping NotifyState(타겟 이름 = WarpTargetName)가 이 타겟을 소비한다
		MotionWarping->AddOrUpdateWarpTargetFromLocationAndRotation(
			WarpTargetName, WarpLocation, WarpTransform.Rotator());
	}

	// 7. 카메라 연출 시작 — 로컬 컨트롤 클라이언트에서만 (서버 전용 인스턴스/타 플레이어 화면 불변).
	//    종료는 EndAbility에서 Clear. 안전망으로 몽타주 길이 + 0.5초 후 자동 해제(MaxDuration).
	if (ActorInfo->IsLocallyControlled() && CurrentAnimSet.CameraMode)
	{
		if (ULeeHeroComponent* Hero = ULeeHeroComponent::FindHeroComponent(Avatar))
		{
			const float CameraMaxDuration = CurrentAnimSet.AttackerMontage->GetPlayLength() + 0.5f;
			Hero->SetAbilityCameraMode(CurrentAnimSet.CameraMode, Handle, Target, CameraMaxDuration);
		}
	}

	// 8. 피해자에게 GameplayEvent 전송 → GA_FinisherVictim 트리거
	//    같은 서버 프레임에 양측 몽타주가 시작되어 타이밍이 동기화된다.
	//    몽타주는 담지 않는다 — 피해자가 자기 스켈레톤 태그로 VictimData를 직접 조회한다.
	//
	//    피해자가 그로기 상태(처형)면 그로기 리액션 어빌리티가 이미 Exclusive_Blocking으로 실행 중일 수 있다.
	//    그러면 같은 슬롯을 쓰는 GA_FinisherVictim이 활성화를 시도조차 못 하고 조용히 막히므로,
	//    이벤트를 보내기 전에 피해자의 기존 실행 중 어빌리티를 전부 정리해 슬롯을 비워준다.
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
	{
		TargetASC->CancelAbilities(nullptr, nullptr, nullptr);
	}

	FGameplayEventData Payload;
	Payload.EventTag = MyTags::Souls::Event_BeFinished;
	Payload.Instigator = Avatar;
	Payload.Target = Target;
	Payload.EventMagnitude = static_cast<float>(CurrentType);
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, Payload.EventTag, Payload);

	// 9. 데미지 타이밍 이벤트 대기 (공격자 몽타주의 AnimNotify가 발사)
	//    OnlyTriggerOnce=false: 다단 히트 몽타주에서 노티파이마다 반복 수신
	UAbilityTask_WaitGameplayEvent* DamageEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			MyTags::Souls::Event_Finisher_Damage,
			/*OptionalExternalOwner*/nullptr,
			/*OnlyTriggerOnce*/false,
			/*OnlyMatchExact*/true);
	DamageEventTask->EventReceived.AddDynamic(this, &ThisClass::OnDamageEventReceived);
	DamageEventTask->ReadyForActivation();

	// 10. 공격자 몽타주 재생 — 모든 종료 경로가 EndAbility로 수렴
	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, CurrentAnimSet.AttackerMontage, /*PlayRate*/1.0f);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
}

void ULeeGameplayAbility_Finisher::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 정상 종료·취소·인터럽트·사망에 의한 강제 취소 — 모든 경로가 여기를 거친다 (무적 해제 보장 지점)
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	if (ASC && InvincibleHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(InvincibleHandle);
	}
	InvincibleHandle.Invalidate();

	if (ASC && AppliedStatusTag.IsValid())
	{
		ASC->RemoveLooseGameplayTag(AppliedStatusTag);
	}
	AppliedStatusTag = FGameplayTag();

	if (AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr)
	{
		if (UMotionWarpingComponent* MotionWarping = Avatar->FindComponentByClass<UMotionWarpingComponent>())
		{
			MotionWarping->RemoveWarpTarget(WarpTargetName);
		}

		// 캡슐 상호 무시 원복 — 시퀀스가 끝나면 두 캐릭터는 다시 서로 밀어내야 한다
		if (AActor* Target = CurrentTarget.Get())
		{
			if (ACharacter* AvatarCharacter = Cast<ACharacter>(Avatar))
			{
				AvatarCharacter->MoveIgnoreActorRemove(Target);
			}
			if (ACharacter* TargetCharacter = Cast<ACharacter>(Target))
			{
				TargetCharacter->MoveIgnoreActorRemove(Avatar);
			}
		}

		// 카메라 복귀 — Clear하면 다음 프레임부터 DefaultCameraMode가 다시 Push되어 블렌딩 복귀.
		// SpecHandle 일치 검사가 있어 카메라를 설정하지 않았던 경우(None)에도 안전하다
		if (ActorInfo->IsLocallyControlled())
		{
			if (ULeeHeroComponent* Hero = ULeeHeroComponent::FindHeroComponent(Avatar))
			{
				Hero->ClearAbilityCameraMode(Handle);
			}
		}
	}

	CurrentTarget.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

float ULeeGameplayAbility_Finisher::ComputeVictimYaw(
	ELeeFinisherType Type, const FVector& AttackerLocation, const FVector& VictimLocation)
{
	const FVector AttackerToVictim = (VictimLocation - AttackerLocation).GetSafeNormal2D();

	if (Type == ELeeFinisherType::Execution)
	{
		// 처형: 피해자가 공격자를 바라본다
		return (-AttackerToVictim).Rotation().Yaw;
	}

	// 암살: 피해자가 공격자 반대 방향(등을 보인 방향)을 바라본다
	return AttackerToVictim.Rotation().Yaw;
}

AActor* ULeeGameplayAbility_Finisher::FindFinisherTarget(
	const FGameplayAbilityActorInfo* ActorInfo, ELeeFinisherType& OutType) const
{
	OutType = ELeeFinisherType::Execution;

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	const FVector AvatarLocation = Avatar->GetActorLocation();

	// 실행 거리 + 여유분으로 폰 오버랩 수집
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LeeFinisherTarget), /*bTraceComplex*/false, Avatar);
	World->OverlapMultiByObjectType(
		Overlaps,
		AvatarLocation,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(ExecuteRadius + 100.0f),
		QueryParams);

	// 후방 원뿔(전체 BehindAngleDeg) 판정용: Dot(타겟 전방, 타겟→공격자) <= -cos(반각)
	const float BehindDotThreshold = -FMath::Cos(FMath::DegreesToRadians(BehindAngleDeg * 0.5f));

	AActor* BestTarget = nullptr;
	ELeeFinisherType BestType = ELeeFinisherType::Execution;
	float BestDistSq = TNumericLimits<float>::Max();

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Candidate == Avatar)
		{
			continue;
		}

		UAbilitySystemComponent* CandidateASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Candidate);
		if (!CandidateASC)
		{
			continue;
		}

		// 이미 피니셔 진행 중(멀티 동시 시도 잠금)이거나 사망한 대상은 제외
		if (CandidateASC->HasMatchingGameplayTag(MyTags::Souls::Status_Finisher_Victim) ||
			CandidateASC->HasMatchingGameplayTag(MyTags::Lyra::Status_Death_Dying) ||
			CandidateASC->HasMatchingGameplayTag(MyTags::Lyra::Status_Death_Dead))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared2D(AvatarLocation, Candidate->GetActorLocation());
		if (DistSq > FMath::Square(ExecuteRadius))
		{
			continue;
		}

		// 분기 지점 ①: 타겟 상태 태그로 피니셔 종류 결정 (그로기가 있으면 처형 우선)
		ELeeFinisherType CandidateType;
		if (CandidateASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy))
		{
			// 처형: 각도 조건 없음 (그로기 상태이므로 전방향 허용)
			CandidateType = ELeeFinisherType::Execution;
		}
		else if (CandidateASC->HasMatchingGameplayTag(MyTags::Souls::Status_Unaware))
		{
			// 암살: 공격자가 타겟 후방 원뿔 안에 있어야 함
			const FVector TargetForward = Candidate->GetActorForwardVector().GetSafeNormal2D();
			const FVector TargetToAttacker = (AvatarLocation - Candidate->GetActorLocation()).GetSafeNormal2D();
			if (FVector::DotProduct(TargetForward, TargetToAttacker) > BehindDotThreshold)
			{
				continue;
			}
			CandidateType = ELeeFinisherType::Assassination;
		}
		else
		{
			continue;
		}

		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Candidate;
			BestType = CandidateType;
		}
	}

	OutType = BestType;
	return BestTarget;
}

void ULeeGameplayAbility_Finisher::OnDamageEventReceived(FGameplayEventData Payload)
{
	AActor* Target = CurrentTarget.Get();
	if (!Target)
	{
		return;
	}

	if (!DamageEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LeeGA_Finisher] DamageEffect가 설정되지 않음. BP에서 GE_FinisherDamage를 지정해주세요."));
		return;
	}

	UAbilitySystemComponent* AttackerASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!AttackerASC || !TargetASC)
	{
		return;
	}

	// SetByCaller 방식으로 피니셔 데미지 적용 (음수 = Health 감소)
	FGameplayEffectContextHandle Context = AttackerASC->MakeEffectContext();
	Context.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

	FGameplayEffectSpecHandle SpecHandle =
		AttackerASC->MakeOutgoingSpec(DamageEffect, GetAbilityLevel(), Context);
	if (SpecHandle.IsValid())
	{
		// Payload.EventMagnitude = 노티파이의 DamageMultiplier (다단 히트 몽타주에서 히트별 비율 조절)
		const float HitDamage = CurrentAnimSet.Damage * Payload.EventMagnitude;
		SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Damage, -HitDamage);
		TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ULeeGameplayAbility_Finisher::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/false);
}

void ULeeGameplayAbility_Finisher::OnMontageInterrupted()
{
	// 인터럽트/외부 취소 — EndAbility가 무적 해제까지 정리한다
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*Replicate*/true, /*WasCancelled*/true);
}
