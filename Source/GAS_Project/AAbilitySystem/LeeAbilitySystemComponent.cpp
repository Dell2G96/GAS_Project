// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeAbilitySystemComponent.h"

#include "Abilities/LeeGameplayAbility.h"
#include "GAS_Project/AAnimation/LeeAnimInstance.h"

ULeeAbilitySystemComponent::ULeeAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
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
				if (LeeAbilityCDO->ActivationPolicy == ELeeAbilityActivationPolicy::WhileInputActive)
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
					if (LeeAbilityCDO->ActivationPolicy == ELeeAbilityActivationPolicy::OnInputTriggered)
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
