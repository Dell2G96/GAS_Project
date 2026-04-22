#include "LeeGameplayAbility_Assassinate.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GAS_Project/MyTags.h"
#include "MotionWarpingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"

ULeeGameplayAbility_Assassinate::ULeeGameplayAbility_Assassinate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ActivationGroup = ELeeAbilityActivationGroup::Exclusive_Blocking;


	WarpTargetName = TEXT("AssassinationTarget");
	//AssassinationEventTag = MyTags::Abilities::Assassination;
	AssassinationEventTag = MyTags::Souls::Ability_Assassination;
	DeathEventTag = MyTags::Souls::GameplayEvent_Death;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = AssassinationEventTag;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void ULeeGameplayAbility_Assassinate::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* TargetActor = TriggerEventData ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
	if (!ValidateTarget(ActorInfo, TargetActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentVictim = TargetActor;
	bSentDeathEvent = false;
	bAddedAssassinatingTag = false;

	ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(MyTags::Souls::Status_Assassinating);
	bAddedAssassinatingTag = true;

	if (AssassinatingStateGE)
	{
		FGameplayEffectContextHandle Context = ActorInfo->AbilitySystemComponent->MakeEffectContext();
		ActiveAssassinatingGEHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(
			AssassinatingStateGE.GetDefaultObject(), 1.0f, Context);
	}

	FGameplayEventData Payload;
	Payload.EventTag = MyTags::Souls::Event_Assassination_Start;
	Payload.Instigator = ActorInfo->AvatarActor.Get();
	Payload.Target = TargetActor;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, MyTags::Souls::Event_Assassination_Start, Payload);

	if (InvincibleGE)
	{
		FGameplayEffectContextHandle Context = ActorInfo->AbilitySystemComponent->MakeEffectContext();
		ActiveInvincibleGEHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(
			InvincibleGE.GetDefaultObject(), 1.0f, Context);
	}

	if (UMotionWarpingComponent* MotionWarpingComp =
		ActorInfo->AvatarActor->FindComponentByClass<UMotionWarpingComponent>())
	{
		const FVector BehindTarget = TargetActor->GetActorLocation() -
			(TargetActor->GetActorForwardVector() * WarpDistanceBehindTarget);

		FTransform WarpTransform;
		WarpTransform.SetLocation(BehindTarget);
		WarpTransform.SetRotation(TargetActor->GetActorForwardVector().Rotation().Quaternion());
		MotionWarpingComp->AddOrUpdateWarpTargetFromTransform(WarpTargetName, WarpTransform);
	}

	if (AssassinationMontage)
	{
		UAbilityTask_PlayMontageAndWait* PlayMontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AssassinationMontage);
		PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
		PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
		PlayMontageTask->ReadyForActivation();
		return;
	}

	SendDeathEventToTarget();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void ULeeGameplayAbility_Assassinate::OnMontageCompleted()
{
	SendDeathEventToTarget();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULeeGameplayAbility_Assassinate::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

bool ULeeGameplayAbility_Assassinate::ValidateTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !IsValid(TargetActor))
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC)
	{
		return false;
	}

	if (TargetASC->HasMatchingGameplayTag(MyTags::Status::Dead) ||
		TargetASC->HasMatchingGameplayTag(MyTags::Status::Executing) ||
		TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Assassinating))
	{
		return false;
	}

	const AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	const float Distance = FVector::Dist(AvatarActor->GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance > ValidationRange)
	{
		return false;
	}

	return IsTargetUnaware(ActorInfo, TargetActor) && IsBehindTarget(ActorInfo, TargetActor);
}

bool ULeeGameplayAbility_Assassinate::IsTargetUnaware(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor) const
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (TargetASC && TargetASC->HasMatchingGameplayTag(MyTags::Souls::Status_Unaware))
	{
		return true;
	}

	const AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	APawn* TargetPawn = Cast<APawn>(TargetActor);
	if (!AvatarActor || !TargetPawn)
	{
		return false;
	}

	AAIController* AIController = Cast<AAIController>(TargetPawn->GetController());
	UAIPerceptionComponent* PerceptionComp = AIController ? AIController->GetPerceptionComponent() : nullptr;
	if (!PerceptionComp)
	{
		return false;
	}

	FActorPerceptionBlueprintInfo PerceptionInfo;
	PerceptionComp->GetActorsPerception(const_cast<AActor*>(AvatarActor), PerceptionInfo);

	for (const FAIStimulus& Stimulus : PerceptionInfo.LastSensedStimuli)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			return false;
		}
	}

	return true;
}

bool ULeeGameplayAbility_Assassinate::IsBehindTarget(const FGameplayAbilityActorInfo* ActorInfo, AActor* TargetActor) const
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

void ULeeGameplayAbility_Assassinate::SendDeathEventToTarget()
{
	if (bSentDeathEvent || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	AActor* InstigatorActor = GetAvatarActorFromActorInfo();
	AActor* TargetActor = CurrentVictim.Get();
	if (!IsValid(InstigatorActor) || !IsValid(TargetActor) || !DeathEventTag.IsValid())
	{
		return;
	}

	FGameplayEventData DeathPayload;
	DeathPayload.EventTag = DeathEventTag;
	DeathPayload.Instigator = InstigatorActor;
	DeathPayload.Target = TargetActor;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, DeathEventTag, DeathPayload);

	bSentDeathEvent = true;
}

void ULeeGameplayAbility_Assassinate::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && ActiveInvincibleGEHandle.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveInvincibleGEHandle);
		ActiveInvincibleGEHandle.Invalidate();
	}

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && ActiveAssassinatingGEHandle.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveAssassinatingGEHandle);
		ActiveAssassinatingGEHandle.Invalidate();
	}

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && bAddedAssassinatingTag)
	{
		ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(MyTags::Souls::Status_Assassinating);
		bAddedAssassinatingTag = false;
	}

	CurrentVictim.Reset();
	bSentDeathEvent = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
