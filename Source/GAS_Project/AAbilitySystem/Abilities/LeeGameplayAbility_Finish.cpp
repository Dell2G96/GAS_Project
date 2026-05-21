#include "LeeGameplayAbility_Finish.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS_Project/AAbilitySystem/LeeGameplayAbilityTargetData_Finish.h"
#include "GAS_Project/AEquipment/LeeEquipmentManagerComponent.h"
#include "GAS_Project/AWeapons/LeeWeaponInstance.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/_Souls/Abilities/LeeSoulsStatSet.h"
#include "GameFramework/Pawn.h"
#include "MotionWarpingComponent.h"

ULeeGameplayAbility_Finish::ULeeGameplayAbility_Finish(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup = ELeeAbilityActivationGroup::Exclusive_Blocking;

	ExecutionEventTag = MyTags::Souls::Ability_Execution;
	LegacyExecutionEventTag = MyTags::Abilities::Execution;
	AssassinationEventTag = MyTags::Souls::Ability_Assassination;
	BeFinishedEventTag = MyTags::Souls::Event_BeFinished;
	DeathEventTag = MyTags::Souls::GameplayEvent_Death;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData ExecutionTriggerData;
		ExecutionTriggerData.TriggerTag = ExecutionEventTag;
		ExecutionTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(ExecutionTriggerData);

		FAbilityTriggerData LegacyExecutionTriggerData;
		LegacyExecutionTriggerData.TriggerTag = LegacyExecutionEventTag;
		LegacyExecutionTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(LegacyExecutionTriggerData);

		FAbilityTriggerData AssassinationTriggerData;
		AssassinationTriggerData.TriggerTag = AssassinationEventTag;
		AssassinationTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(AssassinationTriggerData);
	}
}

void ULeeGameplayAbility_Finish::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* TargetActor = TriggerEventData ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
	const ELeeFinishType FinishType = ResolveFinishType(TriggerEventData);
	if (!ValidateTarget(ActorInfo, TargetActor, FinishType))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ULeeWeaponInstance* WeaponInstance = FindWeaponInstance(ActorInfo);
	FLeeFinishMontagePair SelectedPair;
	if (!WeaponInstance || !WeaponInstance->SelectRandomPair(FinishType, SelectedPair))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentFinishTarget = TargetActor;
	CurrentPair = SelectedPair;
	CurrentFinishType = FinishType;
	bAddedFinishingTag = false;
	bSentOutcome = false;

	if (FinishType == ELeeFinishType::Assassination)
	{
		ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(MyTags::Souls::Status_Assassinating);
	}
	else
	{
		ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(MyTags::Souls::Status_Executing);
	}
	bAddedFinishingTag = true;

	if (FinishingStateGE)
	{
		const FGameplayEffectContextHandle Context = ActorInfo->AbilitySystemComponent->MakeEffectContext();
		ActiveFinishingGEHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(
			FinishingStateGE.GetDefaultObject(), 1.0f, Context);
	}

	if (InvincibleGE)
	{
		const FGameplayEffectContextHandle Context = ActorInfo->AbilitySystemComponent->MakeEffectContext();
		ActiveInvincibleGEHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(
			InvincibleGE.GetDefaultObject(), 1.0f, Context);
	}

	const FTransform AttackerWarpTransform = BuildAttackerWarpTransform(TargetActor, SelectedPair);
	SetupAttackerWarp(ActorInfo, AttackerWarpTransform);
	SendBeFinishedEvent(TargetActor, SelectedPair, AttackerWarpTransform);

	if (SelectedPair.AttackerMontage)
	{
		UAbilityTask_PlayMontageAndWait* PlayMontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, SelectedPair.AttackerMontage);
		PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
		PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		PlayMontageTask->ReadyForActivation();
		return;
	}

	ApplyFinishOutcomeToTarget();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void ULeeGameplayAbility_Finish::OnMontageCompleted()
{
	ApplyFinishOutcomeToTarget();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULeeGameplayAbility_Finish::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

ELeeFinishType ULeeGameplayAbility_Finish::ResolveFinishType(const FGameplayEventData* TriggerEventData) const
{
	if (!TriggerEventData)
	{
		return ELeeFinishType::None;
	}

	if (TriggerEventData->EventTag == AssassinationEventTag ||
		TriggerEventData->EventTag.MatchesTagExact(MyTags::Souls::Ability_Assassination))
	{
		return ELeeFinishType::Assassination;
	}

	if (TriggerEventData->EventTag == ExecutionEventTag ||
		TriggerEventData->EventTag == LegacyExecutionEventTag ||
		TriggerEventData->EventTag.MatchesTagExact(MyTags::Souls::Ability_Execution) ||
		TriggerEventData->EventTag.MatchesTagExact(MyTags::Abilities::Execution))
	{
		return ELeeFinishType::Execution;
	}

	return ELeeFinishType::None;
}

bool ULeeGameplayAbility_Finish::ValidateTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor, ELeeFinishType Type) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !IsValid(TargetActor) || Type == ELeeFinishType::None)
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC)
	{
		return false;
	}

	if (TargetASC->HasMatchingGameplayTag(MyTags::Status::Dead) ||
		TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Executing) ||
		TargetASC->HasMatchingGameplayTag(MyTags::Status::Executing) ||
		TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Assassinating))
	{
		return false;
	}

	const float Distance = FVector::Dist(ActorInfo->AvatarActor->GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance > ValidationRange)
	{
		return false;
	}

	if (Type == ELeeFinishType::Execution)
	{
		return TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Groggy) ||
			TargetASC->HasMatchingGameplayTag(MyTags::Status::Groggy);
	}

	return IsTargetUnaware(TargetActor) && IsBehindTarget(ActorInfo, TargetActor);
}

bool ULeeGameplayAbility_Finish::IsTargetUnaware(AActor* TargetActor) const
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	return TargetASC && TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Unaware);
}

bool ULeeGameplayAbility_Finish::IsBehindTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor) const
{
	const AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!AvatarActor || !IsValid(TargetActor))
	{
		return false;
	}

	const FVector ToInstigator = (AvatarActor->GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal();
	const float DotProduct = FVector::DotProduct(TargetActor->GetActorForwardVector(), ToInstigator);
	const float AngleThreshold = FMath::Cos(FMath::DegreesToRadians(180.0f - AssassinationAngle));
	return DotProduct < AngleThreshold;
}

ULeeWeaponInstance* ULeeGameplayAbility_Finish::FindWeaponInstance(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (ULeeWeaponInstance* AssociatedWeapon = Cast<ULeeWeaponInstance>(GetAssociatedEquipment()))
	{
		return AssociatedWeapon;
	}

	APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	ULeeEquipmentManagerComponent* EquipmentManager = AvatarPawn ? AvatarPawn->FindComponentByClass<ULeeEquipmentManagerComponent>() : nullptr;
	return EquipmentManager ? EquipmentManager->GetFirstInstanceOfType<ULeeWeaponInstance>() : nullptr;
}

FTransform ULeeGameplayAbility_Finish::BuildAttackerWarpTransform(const AActor* TargetActor, const FLeeFinishMontagePair& Pair) const
{
	if (!TargetActor)
	{
		return FTransform::Identity;
	}

	const FVector TargetForward = TargetActor->GetActorForwardVector();
	const FVector TargetRight = TargetActor->GetActorRightVector();
	const FVector WarpLocation = TargetActor->GetActorLocation()
		- TargetForward * Pair.AttackerWarpOffset.X
		+ TargetRight * Pair.AttackerWarpOffset.Y;

	FTransform WarpTransform;
	WarpTransform.SetLocation(WarpLocation);
	WarpTransform.SetRotation(TargetForward.Rotation().Quaternion());
	return WarpTransform;
}

void ULeeGameplayAbility_Finish::SetupAttackerWarp(const FGameplayAbilityActorInfo* ActorInfo, const FTransform& WarpTransform) const
{
	AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!AvatarActor)
	{
		return;
	}

	if (UMotionWarpingComponent* MotionWarpingComp = AvatarActor->FindComponentByClass<UMotionWarpingComponent>())
	{
		MotionWarpingComp->AddOrUpdateWarpTargetFromTransform(AttackerWarpTargetName, WarpTransform);
	}
}

void ULeeGameplayAbility_Finish::SendBeFinishedEvent(AActor* TargetActor, const FLeeFinishMontagePair& Pair, const FTransform& AttackerWarpTransform)
{
	if (!TargetActor || !BeFinishedEventTag.IsValid())
	{
		return;
	}

	FLeeGameplayAbilityTargetData_Finish* FinishData = new FLeeGameplayAbilityTargetData_Finish();
	FinishData->PairID = Pair.PairID;
	FinishData->VictimMontage = Pair.VictimMontage;
	FinishData->AttackerWorldTransform = AttackerWarpTransform;
	FinishData->FinishType = CurrentFinishType;

	FGameplayEventData Payload;
	Payload.EventTag = BeFinishedEventTag;
	Payload.Instigator = GetAvatarActorFromActorInfo();
	Payload.Target = TargetActor;
	Payload.TargetData.Add(FinishData);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, BeFinishedEventTag, Payload);
}

void ULeeGameplayAbility_Finish::ApplyFinishOutcomeToTarget()
{
	if (bSentOutcome || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	AActor* InstigatorActor = GetAvatarActorFromActorInfo();
	AActor* TargetActor = CurrentFinishTarget.Get();
	if (!IsValid(InstigatorActor) || !IsValid(TargetActor))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	bool bAppliedDamage = false;

	if (FinishDamageGE && TargetASC && CurrentActorInfo->AbilitySystemComponent.IsValid())
	{
		float DamageAmount = 0.0f;
		if (const ULeeSoulsStatSet* TargetStatSet = TargetASC->GetSet<ULeeSoulsStatSet>())
		{
			DamageAmount = TargetStatSet->GetMaxHealth() * CurrentPair.DamageRatio;
		}

		if (DamageAmount > 0.0f)
		{
			FGameplayEffectSpecHandle SpecHandle =
				CurrentActorInfo->AbilitySystemComponent->MakeOutgoingSpec(
					FinishDamageGE, 1.0f, CurrentActorInfo->AbilitySystemComponent->MakeEffectContext());

			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(MyTags::Souls::SetByCaller_Damage, -DamageAmount);
				TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				bAppliedDamage = true;
			}
		}
	}

	if (TargetASC)
	{
		RemoveTargetPromptEffects(TargetASC);
	}

	if (!bAppliedDamage && DeathEventTag.IsValid())
	{
		FGameplayEventData DeathPayload;
		DeathPayload.EventTag = DeathEventTag;
		DeathPayload.Instigator = InstigatorActor;
		DeathPayload.Target = TargetActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, DeathEventTag, DeathPayload);
	}

	bSentOutcome = true;
}

void ULeeGameplayAbility_Finish::RemoveTargetPromptEffects(UAbilitySystemComponent* TargetASC) const
{
	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectQuery GroggyQuery;
	GroggyQuery.EffectTagQuery = FGameplayTagQuery::MakeQuery_MatchTag(MyTags::Souls::Status_Groggy);
	TargetASC->RemoveActiveEffects(GroggyQuery);

	FGameplayEffectQuery UnawareQuery;
	UnawareQuery.EffectTagQuery = FGameplayTagQuery::MakeQuery_MatchTag(MyTags::Souls::Status_Unaware);
	TargetASC->RemoveActiveEffects(UnawareQuery);

	TargetASC->RemoveLooseGameplayTag(MyTags::Status::Groggy);
	TargetASC->RemoveLooseGameplayTag(MyTags::Status::Stun);
}

void ULeeGameplayAbility_Finish::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && ActiveInvincibleGEHandle.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveInvincibleGEHandle);
		ActiveInvincibleGEHandle.Invalidate();
	}

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && ActiveFinishingGEHandle.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveFinishingGEHandle);
		ActiveFinishingGEHandle.Invalidate();
	}

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && bAddedFinishingTag)
	{
		ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(MyTags::Souls::Status_Executing);
		ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(MyTags::Souls::Status_Assassinating);
		bAddedFinishingTag = false;
	}

	CurrentFinishTarget.Reset();
	CurrentPair = FLeeFinishMontagePair();
	CurrentFinishType = ELeeFinishType::None;
	bSentOutcome = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
