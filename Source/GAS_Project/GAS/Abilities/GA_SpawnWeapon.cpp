#include "GA_SpawnWeapon.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS_Project/Characters/Player/CPlayerCharacter.h"
#include "GAS_Project/Components/CWeaponComponent.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "GAS_Project/Item/Weapon/CWeapon.h"

UGA_SpawnWeapon::UGA_SpawnWeapon()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void UGA_SpawnWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	//Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	 // 1. 서버 권한 체크
    if (!K2_HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_WeaponSpawn: No Authority"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 2. Avatar 가져오기
    AActor* Avatar = ActorInfo->AvatarActor.Get();
    if (!Avatar)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_WeaponSpawn: Avatar is null"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 3. 캐릭터로 캐스팅
    ACPlayerCharacter* OwnerCharacter = Cast<ACPlayerCharacter>(Avatar);
    if (!OwnerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_WeaponSpawn: Failed to cast to CPlayerCharacter"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 4. 무기 스폰
    UWorld* World = GetWorld();
    if (!World || !WeaponSpawnToClass)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_WeaponSpawn: World or WeaponClass is null"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = OwnerCharacter;
    SpawnParams.Instigator = OwnerCharacter;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    ACWeapon* NewWeapon = World->SpawnActor<ACWeapon>(WeaponSpawnToClass, FTransform::Identity, SpawnParams);
    if (!NewWeapon)
    {
        UE_LOG(LogTemp, Error, TEXT("GA_WeaponSpawn: Failed to spawn weapon"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 5. 소켓에 부착
    USkeletalMeshComponent* OwnerMesh = GetOwningComponentFromActorInfo();
    if (OwnerMesh && NewWeapon)
    {
        NewWeapon->AttachToComponent(
            OwnerMesh,
            FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, EAttachmentRule::KeepWorld, true),
            SocketName
        );
        UE_LOG(LogTemp, Log, TEXT("GA_WeaponSpawn: Weapon attached to socket %s"), *SocketName.ToString());
    }

    // 6. WeaponComponent에 등록
    UCWeaponComponent* WeaponComp = OwnerCharacter->FindComponentByClass<UCWeaponComponent>();
    if (WeaponComp)
    {
        WeaponComp->RegisterSpawnedWeapon(InWeaponTag, NewWeapon, true);
        WeaponComp->CurrentEquippedWeaponTag = InWeaponTag;
        UE_LOG(LogTemp, Log, TEXT("GA_WeaponSpawn: Weapon registered with tag %s"), *InWeaponTag.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_WeaponSpawn: WeaponComponent not found"));
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
