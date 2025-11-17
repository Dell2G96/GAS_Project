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

	UFUNCTION(BlueprintCallable, Category="Comp")
	class ACWeapon* GetWeaponByTag(struct FGameplayTag InWeaponTag) const;

	UFUNCTION(BlueprintCallable, Category="Comp")
	class ACWeapon* GetCurrentWeapon() const;

	UPROPERTY(BlueprintReadOnly, Category="Comp")
	FGameplayTag CurrentWeaponTag;
	
private:
	TMap<struct FGameplayTag, class ACWeapon*> WeaponMap;

		
};
