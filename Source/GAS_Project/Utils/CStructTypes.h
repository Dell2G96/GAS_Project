// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CStructTypes.generated.h"


UENUM(BlueprintType)
enum class ECAbilityInputID : uint8
{
	None							UMETA(DisplayName="None"),
	First_Weapon_Equip				UMETA(DisplayName="First_Weapon_Equip"),
	Second_Weapon_Equip				UMETA(DisplayName="Second_Weapon_Equip"),
	BasicAttack						UMETA(DisplayName="Basic Attack"),
	HeavyAttack						UMETA(DisplayName="Heavy Attack"),
	Guard							UMETA(DisplayName="Guard"),
	Avoid							UMETA(DisplayName="Avoid"),
	TargetLock						UMETA(DisplayName="Target Lock"),
	TargetSwitch					UMETA(DisplayName="Target Switch"),
	AbilityOne						UMETA(DisplayName="Q_Skill"),
	AbilityTwo						UMETA(DisplayName="E_Skill"),
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
	Axe								UMETA(DisplayName="Axe"),
	Mace							UMETA(DisplayName="Mace"),
	Bow								UMETA(DisplayName="Bow"),
	Staff							UMETA(DisplayName="Staff"),
	Max								UMETA(DisplayName="Max")
};

USTRUCT(BlueprintType)
struct FWeaponData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<class UCLinkedAnimLayer> WeaponAnimLayerToLink;
};

USTRUCT(BlueprintType)
struct FWeaponConfig
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category="GAS|WeaponWeapon")
	TSubclassOf<class UGameplayAbility> AbilitiesToGrant;

	UPROPERTY(EditDefaultsOnly, Category="GAS|Weapon")
	FName EquippedSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category="GAS|Weapon")
	TSubclassOf<class UAnimInstance > AnimClass;
	
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
	float BaseMaxStamina;
	
};