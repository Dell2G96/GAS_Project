// 26.11.10 신규 - Enemy AOE(범위) 공격 어빌리티 (모르가나 자기 중심 원형 / 대상 발밑 장판)

#include "LeeGameplayAbility_EnemyAoeAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AWeapons/LeeAoeTelegraphActor.h"

// 서버 실행 · 공격 중 태그(StateTree 종료 판정용) · 기본 수치(자기 중심 원형 기준)
ULeeGameplayAbility_EnemyAoeAttack::ULeeGameplayAbility_EnemyAoeAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup    = ELeeAbilityActivationGroup::Exclusive_Blocking;

	// StateTree의 Lee Activate Ability Task가 태그 폴링으로 종료를 판정하는 경로 (원거리 공격과 같음)
	ActivationOwnedTags.AddTag(MyTags::Souls::Status_Attack_Attacking);

	// 쉴드 중에도 쓸 수 있어야 하므로 Status_Shield_Active로 막지 않는다 (투사체 공격과 다른 점)

	CastEventTag = MyTags::Souls::Event_Attack_Fire;
	Knockback.Offset = FVector(300.0f, 0.0f, 0.0f);
	Knockback.Duration = 0.25f;
	Knockback.GuardScale = 0.5f;
}

// 쿨다운을 걸고, 몽타주가 없으면 바로 원을 깔고 끝낸다. 있으면 Fire 이벤트(또는 몽타주 끝)를 기다린다
void ULeeGameplayAbility_EnemyAoeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bSpawned = false;

	FVector Unused;
	if (!TelegraphActorClass || !FindCenterLocation(Unused) || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		if (!TelegraphActorClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Aoe] %s: TelegraphActorClass가 비어 있습니다. BP에서 B_AoeTelegraph를 지정하세요."), *GetName());
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!Montage)
	{
		SpawnTelegraph();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (CastEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* WaitCast = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, CastEventTag, nullptr, /*OnlyTriggerOnce*/ true, /*OnlyMatchExact*/ true);
		WaitCast->EventReceived.AddDynamic(this, &ThisClass::OnCastEventReceived);
		WaitCast->ReadyForActivation();
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
}

// 몽타주의 Fire 노티파이 — 이 시점에 원을 깐다
void ULeeGameplayAbility_EnemyAoeAttack::OnCastEventReceived(FGameplayEventData Payload)
{
	SpawnTelegraph();
}

// 몽타주 끝 — 노티파이가 없던 몽타주라면 지금 깔고 종료
void ULeeGameplayAbility_EnemyAoeAttack::OnMontageCompleted()
{
	SpawnTelegraph();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

// 피격 등으로 몽타주가 끊김 — 아직 안 깔았으면 깔지 않고 종료
void ULeeGameplayAbility_EnemyAoeAttack::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

// [서버] 데미지 Spec(SetByCaller + AOE 속성 태그)을 만들어 예고 액터를 중심 위치에 스폰한다
void ULeeGameplayAbility_EnemyAoeAttack::SpawnTelegraph()
{
	if (bSpawned || !GetActorInfo().IsNetAuthority())
	{
		return;
	}
	bSpawned = true;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	FVector Center;
	if (!Avatar || !ASC || !FindCenterLocation(Center))
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle;
	if (DamageEffect)
	{
		SpecHandle = ASC->MakeOutgoingSpec(DamageEffect, GetAbilityLevel(), ASC->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Damage, -BaseDamage);
			SpecHandle.Data->AddDynamicAssetTag(MyTags::Souls::DamageType_Attack_Aoe);
			for (const FGameplayTag& Tag : ExtraDamageTypeTags)
			{
				SpecHandle.Data->AddDynamicAssetTag(Tag);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Aoe] %s: DamageEffect가 비어 있어 데미지 없이 깝니다. BP에서 지정하세요."), *GetName());
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, Center);
	ALeeAoeTelegraphActor* Telegraph = GetWorld()->SpawnActorDeferred<ALeeAoeTelegraphActor>(
		TelegraphActorClass, SpawnTransform, Avatar, Cast<APawn>(Avatar), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Telegraph)
	{
		Telegraph->InitAoe(SpecHandle, Avatar, Radius, TelegraphTime, Knockback);
		Telegraph->FinishSpawning(SpawnTransform);
	}

	UE_LOG(LogTemp, Log, TEXT("[Aoe] Cast actor=%s mode=%s center=%s radius=%.0f time=%.2f"), *GetNameSafe(Avatar),
		CenterMode == ELeeAoeCenterMode::Self ? TEXT("Self") : TEXT("Target"), *Center.ToCompactString(), Radius, TelegraphTime);
}

// 중심 = 시전자 또는 AI Focus 대상의 발밑. 아래로 라인트레이스해 바닥 위치를 찾고, 못 찾으면 캡슐 바닥 높이를 쓴다
bool ULeeGameplayAbility_EnemyAoeAttack::FindCenterLocation(FVector& OutLocation) const
{
	APawn* Avatar = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Avatar)
	{
		return false;
	}

	AActor* CenterActor = Avatar;
	if (CenterMode == ELeeAoeCenterMode::Target)
	{
		const AAIController* AIC = Cast<AAIController>(Avatar->GetController());
		CenterActor = AIC ? AIC->GetFocusActor() : nullptr;
		if (!CenterActor)
		{
			return false;
		}
	}

	FVector Feet = CenterActor->GetActorLocation();
	if (const ACharacter* Character = Cast<ACharacter>(CenterActor))
	{
		Feet.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LeeAoeFloor), false, Avatar);
	Params.AddIgnoredActor(CenterActor);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Feet + FVector(0, 0, 50.0f), Feet - FVector(0, 0, 500.0f), ECC_Visibility, Params))
	{
		Feet = Hit.ImpactPoint;
	}

	OutLocation = Feet;
	return true;
}
