// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "LeeGameplayAbility.generated.h"


UENUM(BlueprintType)
enum class ELeeAbilityActivationPolicy : uint8
{
	OnInputTriggered, // Trigger 되었을 경우
	WhileInputActive, // Held 되었을 경우
	OnSpawn			  // avatar가 생성되었을 경우
};

UENUM(BlueprintType)
enum class ELeeAbilityActivationGroup : uint8
{
	// Ability runs independently of all other abilities.
	Independent,

	// Ability is canceled and replaced by other exclusive abilities.
	Exclusive_Replaceable,

	// Ability blocks all other exclusive abilities from activating.
	Exclusive_Blocking,

	MAX	UMETA(Hidden)
};

UCLASS()
class GAS_PROJECT_API ULeeGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	ULeeGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityAdded")
	void K2_OnAbilityAdded();
	
	UFUNCTION(BlueprintCallable, Category="Lee|Ability")
	class ULeeAbilitySystemComponent* GetLeeAbilitySystemComponentFromActorInfo() const ;

	UFUNCTION(BlueprintCallable, Category="Lee|Ability")
	class ALeePlayerController* GetLeePlayerControllerFromActorInfo() const ;

	UFUNCTION(BlueprintCallable, Category="Lee|Ability")
	class ALeeCharacter* GetLeeCharacterFromActorInfo() const ;

	UFUNCTION(BlueprintCallable, Category="Lee|Ability")
	class ULeeHeroComponent* GetHeroComponentFromActorInfo() const ;

	void TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Lyra|Ability", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool CanChangeActivationGroup(ELeeAbilityActivationGroup NewGroup) const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category= "Lee|Ability", meta=(ExpandBoolAsExecs = "ReturnValue"))
	bool ChangeActivationGroup(ELeeAbilityActivationGroup NewGroup);

	
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	ELeeAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }
	ELeeAbilityActivationGroup GetActivationGroup() const { return ActivationGroup; }

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Lee|Ability Activation")
	ELeeAbilityActivationGroup ActivationGroup;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|AbilityActivation")
	ELeeAbilityActivationPolicy ActivationPolicy;

	/** ability costs to apply LeeGameplayAbility separately */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Lee|Costs")
	TArray<TObjectPtr<class ULeeAbilityCost>> AdditionalCosts;
};
