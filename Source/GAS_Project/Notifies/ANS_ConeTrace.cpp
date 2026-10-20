#include "ANS_ConeTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS_Project/MyTags.h"

// 구간 시작 — 이번 공격의 히트 목록 초기화
void UANS_ConeTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) return;

	HitActorsMap.FindOrAdd(MeshComp).Reset();
}

// 구간 종료 — 런타임 데이터 정리
void UANS_ConeTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	HitActorsMap.Remove(MeshComp);
}

// 매 틱 — 원뿔 길이 반경의 구체 Overlap으로 후보를 모은 뒤, 각도/시야 검사를 통과한 대상만 히트 처리
void UANS_ConeTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	UWorld* World = MeshComp->GetWorld();
	if (!OwnerActor || !World) return;

	// 서버 권위에서만 판정 (bServerOnly=false면 어디서나)
	if (bServerOnly && !OwnerActor->HasAuthority()) return;

	TSet<TWeakObjectPtr<AActor>>* HitActors = HitActorsMap.Find(MeshComp);
	if (!HitActors) return;

	const FVector Origin = MeshComp->DoesSocketExist(OriginSocket)
		? MeshComp->GetSocketLocation(OriginSocket)
		: OwnerActor->GetActorLocation();
	const FVector Forward = OwnerActor->GetActorForwardVector().GetSafeNormal2D();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ConeTrace), false);
	Params.AddIgnoredActor(OwnerActor);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(Range), Params);

	if (bDrawDebug)
	{
		DrawDebugCone(World, Origin, Forward, Range, FMath::DegreesToRadians(HalfAngleDeg), FMath::DegreesToRadians(15.f),
			12, FColor::Yellow, false, 0.1f);
	}

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();

		// 한 액터의 여러 컴포넌트가 걸려도, 이번 공격 동안 한 번만 처리
		if (!Candidate || HitActors->Contains(Candidate)) continue;

		if (!IsInsideCone(World, OwnerActor, Origin, Forward, Candidate)) continue;

		HitActors->Add(Candidate);
		HandleHit(OwnerActor, Candidate, Origin);
	}
}

// 대상이 원뿔 안에 있고 시야가 막히지 않았는지 검사 (각도는 수평면 기준)
bool UANS_ConeTrace::IsInsideCone(const UWorld* World, const AActor* OwnerActor, const FVector& Origin,
	const FVector& Forward, AActor* Candidate) const
{
	const FVector TargetLoc = Candidate->GetActorLocation();

	// 각도 검사 — 가드 판정(IsAttackInsideGuardArc)과 같은 Dot vs Cos(반각) 방식
	const FVector ToTarget = (TargetLoc - Origin).GetSafeNormal2D();
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(HalfAngleDeg));
	if (FVector::DotProduct(Forward, ToTarget) < MinDot)
	{
		return false;
	}

	// 시야 검사 — 소켓에서 대상까지 Visibility 트레이스가 다른 물체에 막히면 제외
	if (bCheckLineOfSight)
	{
		FCollisionQueryParams LosParams(SCENE_QUERY_STAT(ConeTraceLOS), false);
		LosParams.AddIgnoredActor(OwnerActor);
		LosParams.AddIgnoredActor(Candidate);

		if (World->LineTraceTestByChannel(Origin, TargetLoc, ECC_Visibility, LosParams))
		{
			return false;
		}
	}

	return true;
}

// 팀 필터 통과 시 ANS_WeaponTrace와 동일 규약으로 공격자에게 GameplayEvent 전송
void UANS_ConeTrace::HandleHit(AActor* OwnerActor, AActor* HitActor, const FVector& Origin) const
{
	// 지정 태도(기본 Hostile)만 처리
	if (IGenericTeamAgentInterface* OwnerTeam = Cast<IGenericTeamAgentInterface>(OwnerActor))
	{
		if (OwnerTeam->GetTeamAttitudeTowards(*HitActor) != TargetTeam)
		{
			return;
		}
	}

	if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor)) return;

	UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	if (!OwnerASC) return;

	// 스윕이 아니라 HitResult가 없으므로, 대상 위치를 충돌 지점으로 하는 HitResult를 직접 만든다
	const FVector HitLoc = HitActor->GetActorLocation();
	const FHitResult HitResult(HitActor, HitActor->GetRootComponent() ? Cast<UPrimitiveComponent>(HitActor->GetRootComponent()) : nullptr,
		HitLoc, (Origin - HitLoc).GetSafeNormal());

	FGameplayEffectContextHandle ContextHandle = OwnerASC->MakeEffectContext();
	ContextHandle.AddHitResult(HitResult);

	FGameplayEventData Payload;
	Payload.Instigator = OwnerActor;
	Payload.Target = HitActor;
	Payload.ContextHandle = ContextHandle;
	Payload.TargetData.Add(new FGameplayAbilityTargetData_SingleTargetHit(HitResult));

	// 공격자(Owner)에게 전송 → LeeGA_AttackMelee의 WaitGameplayEvent가 수신하여 데미지 GE 적용
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, MyTags::Abilities::Enemy::Trace, Payload);
}
