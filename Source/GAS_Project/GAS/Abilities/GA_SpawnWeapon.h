#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_SpawnWeapon.generated.h"

class ACWeapon;

UCLASS()
class GAS_PROJECT_API UGA_SpawnWeapon : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_SpawnWeapon();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

public:
	// 스폰할 무기 "클래스"
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	TSubclassOf<ACWeapon> WeaponClassToSpawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	class ACWeapon* SpawnedWeapon = nullptr;

	// 붙일 소켓 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName SocketName = NAME_None;
	

};
