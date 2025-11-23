// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CStructTypes.generated.h"


UENUM(BlueprintType)
enum class ECabilityInputID : uint8
{
	None							UMETA(DisplayName="None"),
	Equip							UMETA(DisplayName="Equip"),
	UnEquip							UMETA(DisplayName="UnEquip"),
	BasicAttack						UMETA(DisplayName="Basic Attack"),
	HeavyAttack						UMETA(DisplayName="Heavy Attack"),
	Guard							UMETA(DisplayName="Guard"),
	Avoid							UMETA(DisplayName="Avoid"),
	AbilityOne						UMETA(DisplayName="Ability One"),
	AbilityTwo						UMETA(DisplayName="Ability Two"),
	AbilityThree					UMETA(DisplayName="Ability Three"),
	AbilityFour						UMETA(DisplayName="Ability Four"),
	AbilityFive						UMETA(DisplayName="Ability Five"),
	AbilitySix						UMETA(DisplayName="Ability Six"),
	Confirm							UMETA(DisplayName="Confirm"),
	Cancel							UMETA(DisplayName="Cancel")
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None							UMETA(DisplayName="None"),
	Knife							UMETA(DisplayName="Knife"),
	Sword							UMETA(DisplayName="Sword"),
	Axe								UMETA(DisplayName="Axe"),
	Mace							UMETA(DisplayName="Mace"),
	Bow								UMETA(DisplayName="Bow"),
	Staff							UMETA(DisplayName="Staff")
};

USTRUCT(BlueprintType)
struct FWeaponConfig
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category="Weapon")
	TSubclassOf<class UGameplayAbility> AbilitiesToGrant;
	
};

USTRUCT(BlueprintType)
struct FHeroBaseStats : public FTableRowBase
{
	GENERATED_BODY()

public:
	FHeroBaseStats();

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> Class;
	
	UPROPERTY(EditAnywhere)
	float BaseMaxHealth;

	UPROPERTY(EditAnywhere)
	float BaseMaxMana;
	
};