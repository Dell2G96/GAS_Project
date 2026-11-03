#include "ANS_WeaponTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Components/MeshComponent.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AEquipment/LeeEquipmentManagerComponent.h"
#include "GAS_Project/AEquipment/LeeEquipmentInstance.h"
#include "GAS_Project/AEquipment/LeeMeleeWeaponInstance.h"

// 생성자 — 기본 무기 인스턴스 타입을 근접무기로 지정
UANS_WeaponTrace::UANS_WeaponTrace()
{
	WeaponInstanceType = ULeeMeleeWeaponInstance::StaticClass();
}

// 구간 시작 — 무기 메시를 1회 resolve해 캐싱하고 히트 목록 초기화
void UANS_WeaponTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!ShouldRunTrace(MeshComp))
	{
		return;
	}

	FWeaponTraceRuntimeData& Data = RuntimeMap.FindOrAdd(MeshComp);
	Data.WeaponMesh = ResolveWeaponMesh(MeshComp);
	Data.HitActors.Reset();
}

// 구간 끝 — 런타임 데이터 정리
void UANS_WeaponTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	RuntimeMap.Remove(MeshComp);
}

// 매 틱 — 무기 자루→칼끝 사이를 구체 스윕해 히트 처리
void UANS_WeaponTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp) return;

	if (!ShouldRunTrace(MeshComp))
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	FWeaponTraceRuntimeData* DataPtr = RuntimeMap.Find(MeshComp);
	if (!DataPtr)
	{
		return;
	}

	// 무기 메시가 없으면 판정 스킵(무장 해제/스폰 전 등) — 5-a
	UMeshComponent* WeaponMesh = DataPtr->WeaponMesh.Get();
	if (!WeaponMesh)
	{
		return;
	}

	

	// 이번 프레임의 자루/칼끝 위치 (2점)
	const FVector Start = WeaponMesh->GetSocketLocation(WeaponBaseSocket);
	const FVector End   = WeaponMesh->GetSocketLocation(WeaponTipSocket);

	UWorld* World = MeshComp->GetWorld();
	if (!World) return;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponSocketTrace), false);
	if (bIgnoreOwner)
	{
		Params.AddIgnoredActor(OwnerActor);
	}

	// 이미 맞춘 액터는 이번 공격 동안 중복 히트하지 않도록 무시
	for (const TWeakObjectPtr<AActor>& HitActorPtr : DataPtr->HitActors)
	{
		if (AActor* HitActor = HitActorPtr.Get())
		{
			Params.AddIgnoredActor(HitActor);
		}
	}

	TArray<FHitResult> Hits;
	const bool bHit = World->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		TraceChannel,
		FCollisionShape::MakeSphere(TraceRadius),
		Params);

	

	if (bDrawDebug)
	{
		DrawDebugLine(World, Start, End, FColor::Cyan, false, 0.1f, 0, 1.0f);
		DrawDebugSphere(World, End, TraceRadius, 12, bHit ? FColor::Red : FColor::Green, false, 0.1f);
	}

	if (bHit)
	{
		for (const FHitResult& HR : Hits)
		{
			AActor* HitActor = HR.GetActor();
			if (!HitActor) continue;

			// 중복 히트 방지 등록 후 이벤트 처리
			DataPtr->HitActors.Add(HitActor);
			HandleHit(OwnerActor, HR);
		}
	}
}

// 서버 권위에서만 판정(멀티 권장). bServerOnly=false면 어디서나 실행 (에디터 프리뷰 월드는 제외)
bool UANS_WeaponTrace::ShouldRunTrace(const USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp) return false;

	const UWorld* World = MeshComp->GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return false;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return false;

	if (!bServerOnly) return true;

	return OwnerActor->HasAuthority();
}

// LeeEquipment → 무기 인스턴스 → 스폰 무기 액터 → 두 소켓을 모두 가진 메시 반환(방패 등 자동 제외)
UMeshComponent* UANS_WeaponTrace::ResolveWeaponMesh(USkeletalMeshComponent* MeshComp) const
{
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) return nullptr;

	if (ULeeEquipmentManagerComponent* EquipMgr = Owner->FindComponentByClass<ULeeEquipmentManagerComponent>())
	{
		const TArray<ULeeEquipmentInstance*> Instances = EquipMgr->GetEquipmentInstancesOfType(ULeeEquipmentInstance::StaticClass());

		for (ULeeEquipmentInstance* Inst : Instances)
		{
			if (!Inst) continue;

			const TArray<AActor*> SpawnedActors = Inst->GetSpawnedActors();

			for (AActor* WeaponActor : SpawnedActors)
			{
				if (!WeaponActor) continue;

				TArray<UMeshComponent*> Meshes;
				WeaponActor->GetComponents<UMeshComponent>(Meshes);

				for (UMeshComponent* M : Meshes)
				{
					const bool bHasBase = M && M->DoesSocketExist(WeaponBaseSocket);
					const bool bHasTip  = M && M->DoesSocketExist(WeaponTipSocket);

					// 자루/칼끝 소켓을 모두 가진 메시만 무기로 인정
					if (bHasBase && bHasTip)
					{
						return M;
					}
				}
			}
		}
	}

	// [폴백] 장비 시스템에서 못 찾으면, 소유 액터(적)에 직접 붙은 무기 메시 컴포넌트에서 탐색
	// (무기를 EquipmentManager가 아니라 BP 컴포넌트로 손 소켓에 붙인 경우 대응)
	{
		TArray<UMeshComponent*> OwnerMeshes;
		Owner->GetComponents<UMeshComponent>(OwnerMeshes);

		for (UMeshComponent* M : OwnerMeshes)
		{
			if (M == MeshComp) continue; // 캐릭터 본체 메시는 제외

			const bool bHasBase = M && M->DoesSocketExist(WeaponBaseSocket);
			const bool bHasTip  = M && M->DoesSocketExist(WeaponTipSocket);

			if (bHasBase && bHasTip)
			{
				return M;
			}
		}
	}

	return nullptr;
}

// 팀 필터 통과 시 ANS_ToggleTrace와 동일 규약으로 공격자에게 GameplayEvent 전송
void UANS_WeaponTrace::HandleHit(AActor* OwnerActor, const FHitResult& HitResult) const
{
	AActor* HitActor = HitResult.GetActor();
	if (!HitActor) return;

	UE_LOG(LogTemp, Log, TEXT("[WeaponTrace] HandleHit: 대상=%s"), *GetNameSafe(HitActor));

	// 지정 태도(기본 Hostile)만 처리
	if (IGenericTeamAgentInterface* OwnerTeam = Cast<IGenericTeamAgentInterface>(OwnerActor))
	{
		if (OwnerTeam->GetTeamAttitudeTowards(*HitActor) != TargetTeam)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WeaponTrace] STOP@Team: %s 는 TargetTeam이 아님(팀ID/적대 설정 확인)"), *GetNameSafe(HitActor));
			return;
		}
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!TargetASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponTrace] STOP@TargetASC=null: %s 에 ASC 없음"), *GetNameSafe(HitActor));
		return;
	}

	UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	if (!OwnerASC) return;

	UE_LOG(LogTemp, Log, TEXT("[WeaponTrace] SEND EVENT Abilities.Enemy.Trace → %s (어빌리티가 수신 대기 중이어야 함)"), *GetNameSafe(OwnerActor));

	// ANS_ToggleTrace와 동일: Instigator=공격자, TargetData=HitResult, 태그=Abilities.Enemy.Trace
	FGameplayEffectContextHandle ContextHandle = OwnerASC->MakeEffectContext();
	ContextHandle.AddHitResult(HitResult);

	FGameplayEventData Payload;
	Payload.Instigator = OwnerActor;
	Payload.Target = HitActor;
	Payload.ContextHandle = ContextHandle;

	FGameplayAbilityTargetData_SingleTargetHit* TargetData = new FGameplayAbilityTargetData_SingleTargetHit(HitResult);
	Payload.TargetData.Add(TargetData);

	// 공격자(Owner)에게 전송 → GA_AttackMelee의 WaitGameplayEvent가 수신하여 데미지 GE 적용
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, MyTags::Abilities::Enemy::Trace, Payload);
}

