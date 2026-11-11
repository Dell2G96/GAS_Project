// 26.11.11 신규 - 공용 피격 이펙트·사운드 GameplayCue (GameplayCue.Souls.Hit.Impact)

#include "LeeGameplayCueNotify_HitImpact.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GAS_Project/MyTags.h"

// 생성자 — Cue 태그 지정
ULeeGameplayCueNotify_HitImpact::ULeeGameplayCueNotify_HitImpact()
{
	GameplayCueTag = MyTags::Souls::GameplayCue_Hit_Impact;
}

// 피격 — 맞은 지점에 이펙트(ImpactScale 크기)와 사운드 재생
bool ULeeGameplayCueNotify_HitImpact::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!MyTarget)
	{
		return false;
	}

	const FVector HitLocation = Parameters.Location.IsZero() ? MyTarget->GetActorLocation() : FVector(Parameters.Location);
	const FVector HitDirection = FVector(Parameters.Normal).GetSafeNormal(UE_SMALL_NUMBER, MyTarget->GetActorForwardVector());

	if (ImpactSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(MyTarget, ImpactSystem, HitLocation, HitDirection.Rotation(), ImpactScale);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(MyTarget, ImpactSound, HitLocation);
	}
	return true;
}
