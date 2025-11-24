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
	UCWeaponComponent();
protected:
	virtual void BeginPlay() override;
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CurrentEquippedWeaponTag();

	// 복제되는 현재 무기 (서버 권한)
	UPROPERTY(ReplicatedUsing=OnRep_CurrentWeapon, BlueprintReadOnly, Category="Weapon")
	class ACWeapon* CurrentWeapon;

	UFUNCTION()
	void OnRep_CurrentWeapon(class ACWeapon* OldWeapon);

public:
	// Ability에서 호출할 함수들
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void AttachWeapon(class ACWeapon* NewWeapon);

	UFUNCTION(BlueprintCallable, Category="Weapon")
	void DetachWeapon();

	UFUNCTION(BlueprintPure, Category="Weapon")
	bool HasWeaponEquipped() const { return CurrentWeapon != nullptr; }

	//Legacy Code
	// 스폰된 무기 등록
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void RegisterSpawnedWeapon(FGameplayTag InWeaponTag, ACWeapon* InWeapon, bool bRegister);

	// 무기 가져오기 (태그로)
	UFUNCTION(BlueprintPure, Category="Weapon")
	ACWeapon* GetWeaponByTag(FGameplayTag WeaponTag) const;

	// 현재 장착된 무기 태그
	UPROPERTY(ReplicatedUsing=OnRep_CurrentEquippedWeaponTag, BlueprintReadWrite, Category="Weapon")
	FGameplayTag CurrentEquippedWeaponTag;

	// 현재 장착된 무기 가져오기
	UFUNCTION(BlueprintPure, Category="GAS|Weapon")
	class ACWeapon* GetCurrentWeapon() const;

private:
	UPROPERTY()
	USkeletalMeshComponent* OwnerMesh;

	void CacheOwnerMesh();
	
private:
	TMap<struct FGameplayTag, class ACWeapon*> WeaponMap;
	
};
