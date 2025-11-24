// Fill out your copyright notice in the Description page of Project Settings.


#include "GA_Equip.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/GAS/Abilities/GA_SpawnWeapon.h"
#include "GA_SpawnWeapon.h"
#include "GAS_Project/Components/CWeaponComponent.h"
#include "GAS_Project/Item/Weapon/CWeapon.h"

UGA_Equip::UGA_Equip()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void UGA_Equip::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AActor* Avatar = ActorInfo->AvatarActor.Get();
    if (!Avatar)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UCWeaponComponent* WeaponComp = Avatar->FindComponentByClass<UCWeaponComponent>();
    if (!WeaponComp)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_EquipWeapon: WeaponComponent not found"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 이미 해당 무기가 스폰되어 있는지 확인
    ACWeapon* ExistingWeapon = WeaponComp->GetWeaponByTag(WeaponTagToEquip);
    
    if (!ExistingWeapon)
    {
        // 무기가 없으면 GA_WeaponSpawn 호출
        if (!WeaponSpawnAbilityClass)
        {
            UE_LOG(LogTemp, Error, TEXT("GA_EquipWeapon: WeaponSpawnAbilityClass not set"));
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }

        // WeaponSpawn Ability를 활성화
        FGameplayAbilitySpec SpawnSpec(WeaponSpawnAbilityClass, 1, INDEX_NONE, this);
        
        // WeaponSpawn Ability에 태그 전달 (블루프린트에서 설정된 InWeaponTag 사용)
        ActorInfo->AbilitySystemComponent->GiveAbilityAndActivateOnce(SpawnSpec);
        
        UE_LOG(LogTemp, Log, TEXT("GA_EquipWeapon: Called GA_WeaponSpawn for tag %s"), *WeaponTagToEquip.ToString());
    }
    else
    {
        // 무기가 이미 있으면 단순히 CurrentEquippedWeaponTag 변경
        WeaponComp->CurrentEquippedWeaponTag = WeaponTagToEquip;
        UE_LOG(LogTemp, Log, TEXT("GA_EquipWeapon: Weapon already exists, just equipped"));
    }

    // 애니메이션 재생
    if (EquipMontage)
    {
        UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this,
            NAME_None,
            EquipMontage,
            1.0f
        );

        Task->OnCompleted.AddDynamic(this, &UGA_Equip::OnMontageCompleted);
        Task->OnCancelled.AddDynamic(this, &UGA_Equip::OnMontageCancelled);
        Task->OnInterrupted.AddDynamic(this, &UGA_Equip::OnMontageCancelled);

        Task->ReadyForActivation();
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}


void UGA_Equip::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Equip::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Equip::OnMontageCancelled()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);

}
