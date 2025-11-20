#include "GA_SpawnWeapon.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS_Project/GAS/CAbilitySystemStatics.h"
#include "GAS_Project/Item/Weapon/CWeapon.h"

UGA_SpawnWeapon::UGA_SpawnWeapon()
{
	// (선택) 에셋 태그 설정
	FGameplayTagContainer Tags;
	// Tags.AddTag(WeaponTagToRegister);  // 유효한 태그가 있을 때만 추가하세요
	SetAssetTags(Tags);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_SpawnWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// (선택) 자원/쿨다운을 쓰고 있다면 Commit
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !WeaponClassToSpawn)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	UWorld* World = ActorInfo->AvatarActor->GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// 서버 권한에서만 스폰 권장(리플리케이션 의도 시)
	// if (!ActorInfo->IsNetAuthority()) { EndAbility(Handle, ActorInfo, ActivationInfo, false, true); return; }

	const FTransform SpawnTM; // 바로 어태치할 거라 Identity로 충분
	FActorSpawnParameters Params;
	Params.Owner = ActorInfo->AvatarActor.Get();
	Params.Instigator = Cast<APawn>(ActorInfo->AvatarActor.Get());
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ACWeapon* NewWeapon = World->SpawnActor<ACWeapon>(WeaponClassToSpawn, SpawnTM, Params);
	if (!IsValid(NewWeapon))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// 필요 시 리플리케이션 설정
	// NewWeapon->SetReplicates(true);

	// 메시에 붙이기
	if (USkeletalMeshComponent* ParentComp = Cast<USkeletalMeshComponent>(GetOwningComponentFromActorInfo()))
	{
		const FAttachmentTransformRules AttachRules(
			EAttachmentRule::SnapToTarget,   // Location
			EAttachmentRule::KeepRelative,   // Rotation
			EAttachmentRule::KeepWorld,      // Scale
			true                             // bWeldSimulatedBodies
		);

		NewWeapon->AttachToComponent(ParentComp, AttachRules, SocketName);
	}

	// ✅ 인스턴스를 별도 변수에 저장 (클래스 변수에 대입 금지!)
	SpawnedWeapon = NewWeapon;

	// 작업 끝
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
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
