// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "CWeaponComponent.generated.h"


UCLASS(Blueprintable)
class GAS_PROJECT_API UCWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="GAS|Combat")
	void EquipWeapon(TSubclassOf<class ACWeapon> NewWeapon);
	
	UFUNCTION(BlueprintCallable, Category="GAS|Combat")
	void UnEquipWeapon(TSubclassOf<class ACWeapon> NewWeapon);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Combat", meta=(AllowPrivateAccess="true"))
	TSubclassOf<class ACWeapon> WeaponToEquip;

	UPROPERTY(EditDefaultsOnly, Category="GAS|Combat")
	FName SocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category="GAS|Combat")
	class UAnimMontage* EquipMontage;

	UPROPERTY(EditDefaultsOnly, Category="GAS|Combat")
	class UAnimMontage* UnEquipMontage;
	
	UPROPERTY()
	class ACPlayerCharacter* OwnerCharacter;

	
	
public:	
	UFUNCTION(BlueprintCallable, Category="Comp")
	void RegisterSpawnedWeapon(struct FGameplayTag InWeaponTag, class ACWeapon* InWeapon , bool bRegister = false);

	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	ACWeapon* GetCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const;

	UPROPERTY(BlueprintReadWrite, Category = "Warrior|Combat")
	FGameplayTag CurrentEquippedWeaponTag;

	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat")
	ACWeapon* GetCharacterCurrentEquippedWeapon() const;
	
private:
	TMap<struct FGameplayTag, class ACWeapon*> WeaponMap;
	
};
