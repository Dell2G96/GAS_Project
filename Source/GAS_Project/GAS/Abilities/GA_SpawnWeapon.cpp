#include "GA_SpawnWeapon.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "GAS_Project/Item/Weapon/CWeapon.h"

UGA_SpawnWeapon::UGA_SpawnWeapon()
{
	
}

void UGA_SpawnWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
}

void UGA_SpawnWeapon::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// (선택) 취소 시 정리 로직
	// if (bWasCancelled && SpawnedWeapon.IsValid()) { SpawnedWeapon->Destroy(); }

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
