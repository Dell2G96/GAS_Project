// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "CWeaponComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAS_PROJECT_API UCWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

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
