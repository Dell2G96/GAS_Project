// 26.11.08 신규 - 마법 쉴드 피격 GameplayCue (GameplayCue.Souls.Shield.Hit)

#include "LeeGameplayCueNotify_ShieldHit.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GAS_Project/MyTags.h"
#include "GAS_Project/AFeedBack/LeeGameplayCue_MagicShield.h"

// 생성자 — Cue 태그 지정
ULeeGameplayCueNotify_ShieldHit::ULeeGameplayCueNotify_ShieldHit()
{
	GameplayCueTag = MyTags::Souls::GameplayCue_Shield_Hit;
}

// 피격 — 쉴드 금 갱신 + 번쩍임, 남은 비율에 따라 파편/파괴 이펙트 생성
bool ULeeGameplayCueNotify_ShieldHit::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!MyTarget)
	{
		return false;
	}

	const float Remaining = FMath::Clamp(Parameters.RawMagnitude, 0.0f, 1.0f);
	const bool bBroken = Remaining <= 0.0f;

	// 26.11.08 수정 - 맞은 방향(GA가 Normal에 넣음)에 금을 새기고, 이펙트도 그 표면 위치에서 낸다
	const FVector HitDirection = FVector(Parameters.Normal).GetSafeNormal(UE_SMALL_NUMBER, MyTarget->GetActorForwardVector());
	FVector EffectLocation = MyTarget->GetActorLocation();

	if (ALeeGameplayCue_MagicShield* Shield = ALeeGameplayCue_MagicShield::FindOnTarget(MyTarget))
	{
		Shield->AddCrackImpact(HitDirection, 1.0f - Remaining);
		Shield->PlayHitFlash();
		EffectLocation = Shield->GetSurfacePoint(HitDirection);

		// 깨짐 — 구를 유리 조각으로 흩뿌린다 (공격 반대쪽으로 더 밀려 나감)
		if (bBroken)
		{
			Shield->Shatter(-HitDirection);
		}
	}

	UNiagaraSystem* System = bBroken ? BreakSystem.Get() : CrackSystem.Get();
	if (System)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(MyTarget, System, EffectLocation, HitDirection.Rotation());
	}
	return true;
}
