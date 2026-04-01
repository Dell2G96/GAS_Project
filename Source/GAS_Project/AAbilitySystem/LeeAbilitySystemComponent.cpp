// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeAbilitySystemComponent.h"

#include "Abilities/LeeGameplayAbility.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/AAnimation/LeeAnimInstance.h"

ULeeAbilitySystemComponent::ULeeAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	FMemory::Memzero(ActivationGroupCounts, sizeof(ActivationGroupCounts));
}

void ULeeAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	check(ActorInfo);
	check(InOwnerActor);

	const bool bHasNewPawnAvatar = Cast<APawn>(InAvatarActor) && (InAvatarActor != ActorInfo->AvatarActor);

	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (bHasNewPawnAvatar)
	{
		if (ULeeAnimInstance* LyraAnimInst = Cast<ULeeAnimInstance>(ActorInfo->GetAnimInstance()))
		{
			LyraAnimInst->InitializeWithAbilitySystem(this);
		}
		TryActivateAbilitiesOnSpawn();
	}

}

bool ULeeAbilitySystemComponent::IsActivationGroupBlocked(ELeeAbilityActivationGroup Group) const
{
	bool bBlocked = false;

	switch (Group)
	{
	case ELeeAbilityActivationGroup::Independent:
		// Independent abilities are never blocked.
			bBlocked = false;
		break;

	case ELeeAbilityActivationGroup::Exclusive_Replaceable:
	case ELeeAbilityActivationGroup::Exclusive_Blocking:
		// Exclusive abilities can activate if nothing is blocking.
		bBlocked = (ActivationGroupCounts[(uint8)ELeeAbilityActivationGroup::Exclusive_Blocking] > 0);
		break;

	default:
		checkf(false, TEXT("IsActivationGroupBlocked: Invalid ActivationGroup [%d]\n"), (uint8)Group);
		break;
	}

	return bBlocked;
}

void ULeeAbilitySystemComponent::AddAbilityToActivationGroup(ELeeAbilityActivationGroup Group,
	ULeeGameplayAbility* LeeAbility)
{
	check(LeeAbility);
	check(ActivationGroupCounts[(uint8)Group] < INT32_MAX);

	ActivationGroupCounts[(uint8)Group]++;

	const bool bReplicateCancelAbility = false;

	switch (Group)
	{
	case ELeeAbilityActivationGroup::Independent:
		// Independent abilities do not cancel any other abilities.
			break;

	case ELeeAbilityActivationGroup::Exclusive_Replaceable:
	case ELeeAbilityActivationGroup::Exclusive_Blocking:
		CancelActivationGroupAbilities(ELeeAbilityActivationGroup::Exclusive_Replaceable, LeeAbility, bReplicateCancelAbility);
		break;

	default:
		checkf(false, TEXT("AddAbilityToActivationGroup: Invalid ActivationGroup [%d]\n"), (uint8)Group);
		break;
	}

	const int32 ExclusiveCount = ActivationGroupCounts[(uint8)ELeeAbilityActivationGroup::Exclusive_Replaceable] + ActivationGroupCounts[(uint8)ELeeAbilityActivationGroup::Exclusive_Blocking];
	if (!ensure(ExclusiveCount <= 1))
	{
		UE_LOG(LogLee, Error, TEXT("AddAbilityToActivationGroup: Multiple exclusive abilities are running."));
	}
}

void ULeeAbilitySystemComponent::CancelActivationGroupAbilities(ELeeAbilityActivationGroup Group,
	ULeeGameplayAbility* IgnoreAbility, bool bReplicateCancelAbility)
{
	auto ShouldCancelFunc = [this, Group, IgnoreAbility](const ULeeGameplayAbility* LeeAbility, FGameplayAbilitySpecHandle Handle)
	{
		return ((LeeAbility->GetActivationGroup() == Group) && (LeeAbility != IgnoreAbility));
	};

	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void ULeeAbilitySystemComponent::CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc,
	bool bReplicateCancelAbility)
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (!AbilitySpec.IsActive())
		{
			continue;
		}

		ULeeGameplayAbility* LeeAbilityCDO = Cast<ULeeGameplayAbility>(AbilitySpec.Ability);
		if (!LeeAbilityCDO)
		{
			UE_LOG(LogLee, Error, TEXT("CancelAbilitiesByFunc: Non-LeeGameplayAbility %s was Granted to ASC. Skipping."), *AbilitySpec.Ability.GetName());
			continue;
		}

		PRAGMA_DISABLE_DEPRECATION_WARNINGS
				ensureMsgf(AbilitySpec.Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced, TEXT("CancelAbilitiesByFunc: All Abilities should be Instanced (NonInstanced is being deprecated due to usability issues)."));
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
			
				// Cancel all the spawned instances.
				TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
		for (UGameplayAbility* AbilityInstance : Instances)
		{
			ULeeGameplayAbility* LeeAbilityInstance = CastChecked<ULeeGameplayAbility>(AbilityInstance);

			if (ShouldCancelFunc(LeeAbilityInstance, AbilitySpec.Handle))
			{
				if (LeeAbilityInstance->CanBeCanceled())
				{
					LeeAbilityInstance->CancelAbility(AbilitySpec.Handle, AbilityActorInfo.Get(), LeeAbilityInstance->GetCurrentActivationInfo(), bReplicateCancelAbility);
				}
				else
				{
					UE_LOG(LogLee, Error, TEXT("CancelAbilitiesByFunc: Can't cancel ability [%s] because CanBeCanceled is false."), *LeeAbilityInstance->GetName());
				}
			}
		}
	}
}


void ULeeAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
			{
				InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
			}
		}
	}
}

void ULeeAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
			{
				// Released에 추가하고, Held에서는 제거해준다
				InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.Remove(AbilitySpec.Handle);
			}
		}
	}
}

void ULeeAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability && !AbilitySpec->IsActive())
			{
				const ULeeGameplayAbility* LeeAbilityCDO = CastChecked<ULeeGameplayAbility>(AbilitySpec->Ability);

				// Todo
				if (LeeAbilityCDO->GetActivationPolicy() == ELeeAbilityActivationPolicy::WhileInputActive)
				{
					AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
				}
			}
		}
	}
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = true;

				if (AbilitySpec->IsActive())
				{
					// 이미 Ability가 활성화되어 있을 경우, Input Event(InputPressed)만 호출
					// - AbilitySpecInputPressed 확인
					AbilitySpecInputPressed(*AbilitySpec);
				}
				else
				{
					const ULeeGameplayAbility* LeeAbilityCDO = CastChecked<ULeeGameplayAbility>(AbilitySpec->Ability);

					// ActivationPolicy가 OnInputTriggered 속성이면 활성화로 등록
					if (LeeAbilityCDO->GetActivationPolicy() == ELeeAbilityActivationPolicy::OnInputTriggered)
					{
						AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
					}
				}
			}
		}
	}
	// 등록된 AbilitiesToActivate를 한꺼번에 등록 시작:
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		// 모든 것이 잘 진행되었다면, CallActivate 호출로 BP의 Activate 노드가 실행될 것임
		TryActivateAbility(AbilitySpecHandle);
	}

	// 이번 프레임에 Release되었다면, 관련 GameplayAbility 처리:
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = false;
				if (AbilitySpec->IsActive())
				{
					AbilitySpecInputReleased(*AbilitySpec);
				}
			}
		}
	}
	// InputHeldSpecHandles은 InputReleasedSpecHandles 추가될때 제거된다!
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void ULeeAbilitySystemComponent::RemoveAbilityFromActivationGroup(ELeeAbilityActivationGroup Group,
	ULeeGameplayAbility* LeeAbility)
{
	check(LeeAbility);

	if (ActivationGroupCounts[(uint8)Group] > 0)
	{
		ActivationGroupCounts[(uint8)Group]--;
	}
	else
	{
		UE_LOG(LogLee, Warning, TEXT("RemoveAbilityFromActivationGroup: ActivationGroupCount is already 0 for Group [%d], Ability [%s]"),
			(uint8)Group, *LeeAbility->GetName());
	}
}

void ULeeAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (const ULeeGameplayAbility* LeeAbilityCDO = Cast<ULeeGameplayAbility>(AbilitySpec.Ability))
		{
			LeeAbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
		}
	}
}

