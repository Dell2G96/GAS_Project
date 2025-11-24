// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CGameplayAbility.h"
#include "GA_Equip.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UGA_Equip : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Equip();

	// WeaponSpawn Ability 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	TSubclassOf<class UGA_SpawnWeapon> WeaponSpawnAbilityClass;
	
	// 장착 애니메이션
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation")
	UAnimMontage* EquipMontage;

	// 장착할 무기 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FGameplayTag WeaponTagToEquip;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();
};
