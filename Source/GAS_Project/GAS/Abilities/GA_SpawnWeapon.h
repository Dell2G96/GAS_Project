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

	// 스폰할 무기 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	TSubclassOf<ACWeapon> WeaponSpawnToClass;

	// 무기 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FGameplayTag InWeaponTag;

	// 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FName SocketName = TEXT("WeaponSocket");

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;
};
