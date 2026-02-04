// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GAS_Project/Utils/CStructTypes.h"
#include "CWeaponComponent.generated.h"


// 무기 한 칸을 표현하는 구조체
USTRUCT()
struct FWeaponEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag WeaponTag;

	UPROPERTY()
	class ACWeapon* Weapon = nullptr;

	FGameplayTag GetCurretWeaponTag()
	{
		return WeaponTag;
	}
};

UCLASS(Blueprintable)
class GAS_PROJECT_API UCWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCWeaponComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	inline FGameplayTag GetCurrentEquippedWeaponTag() const { return CurrentEquippedWeaponTag; }

	UFUNCTION(BlueprintCallable, Category="GAS|Comp")
	void RegisterSpawnedWeapon(FGameplayTag InWeaponTag, ACWeapon* InWeapon, bool bRegister = false);

	UFUNCTION(BlueprintCallable, Category="GAS|Combat")
	ACWeapon* GetCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const;

	UPROPERTY(Replicated, BlueprintReadWrite, Category="GAS|Combat")
	FGameplayTag CurrentEquippedWeaponTag;

	UFUNCTION(BlueprintCallable, Category="GAS|Combat")
	ACWeapon* GetCharacterCurrentEquippedWeapon() const;

	UFUNCTION(BlueprintCallable, Category="GAS|Combat")
	ACWeapon* GetPlayerCurrentEquippedWeapon(FGameplayTag InWeaponTagToGet) const;

protected:

	// 배열로 복제할 데이터
	UPROPERTY(ReplicatedUsing=OnRep_WeaponEntries)
	TArray<FWeaponEntry> WeaponEntries;

	UFUNCTION()
	void OnRep_WeaponEntries();

private:
	// 이건 네가 쓰던 검색용 맵 (복제X, 로컬 캐시용)
	TMap<FGameplayTag, ACWeapon*> WeaponMap;
};
