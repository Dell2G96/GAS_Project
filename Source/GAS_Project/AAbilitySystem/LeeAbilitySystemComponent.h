// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Abilities/LeeGameplayAbility.h"
#include "LeeAbilitySystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAS_PROJECT_API ULeeAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	ULeeAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	bool IsActivationGroupBlocked(ELeeAbilityActivationGroup Group) const;
	void AddAbilityToActivationGroup(ELeeAbilityActivationGroup Group, ULeeGameplayAbility* LeeAbility);

	void CancelActivationGroupAbilities(ELeeAbilityActivationGroup Group, ULeeGameplayAbility* IgnoreAbility, bool bReplicateCancelAbility);

	typedef TFunctionRef<bool(const ULeeGameplayAbility* LyraAbility, FGameplayAbilitySpecHandle Handle)> TShouldCancelAbilityFunc;
	void CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility);	
	
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

	void RemoveAbilityFromActivationGroup(ELeeAbilityActivationGroup Group, ULeeGameplayAbility* LeeAbility);

	

	void TryActivateAbilitiesOnSpawn();
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

	int32 ActivationGroupCounts[(uint8)ELeeAbilityActivationGroup::MAX];
	
};

