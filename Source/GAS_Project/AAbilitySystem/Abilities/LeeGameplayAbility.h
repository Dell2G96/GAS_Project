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
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void SetCanBeCanceled(bool bCanBeCanceled) override;

	UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "OnAbilityAdded")
	void K2_OnAbilityAdded();
	
	UFUNCTION(BlueprintCallable, Category="Lee|Ability")
	class ULeeAbilitySystemComponent* GetLeeAbilitySystemComponentFromActorInfo() const ;

	UFUNCTION(BlueprintCallable, Category="Lee|Ability")
	class ALeePlayerController* GetLeePlayerControllerFromActorInfo() const ;

	UFUNCTION(BlueprintCallable, Category="Lee|Ability")
	AController* GetControllerFromActorInfo() const ; 

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
	/**
	 * 26.11.17 추가 - [로컬] 입력 액션에 매핑된 키 중 하나라도 지금 눌려 있는지.
	 * 입력 트리거(Pressed 등)와 상관없이 실제 키 상태로 판단한다 — 홀드 판정(강공격 차지, 달리기)용.
	 * 직접 조종하는 클라이언트에서만 의미가 있고, 서버의 원격 플레이어 인스턴스에서는 항상 false.
	 */
	bool IsInputActionHeld(const class UInputAction* InputAction) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Lee|Ability Activation")
	ELeeAbilityActivationGroup ActivationGroup;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lee|AbilityActivation")
	ELeeAbilityActivationPolicy ActivationPolicy;

	/** ability costs to apply LeeGameplayAbility separately */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Lee|Costs")
	TArray<TObjectPtr<class ULeeAbilityCost>> AdditionalCosts;
};
