// 26.11.08 신규 - 마법 쉴드 피격 GameplayCue (GameplayCue.Souls.Shield.Hit)

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "LeeGameplayCueNotify_ShieldHit.generated.h"

class UNiagaraSystem;

/**
 * 쉴드가 맞을 때마다 실행되는 Static Cue.
 * RawMagnitude = 남은 쉴드 비율(0~1), Normal = 맞은 방향(월드).
 * 쉴드 표시 Cue의 맞은 자리에 금을 새겨 (1 - 비율)만큼 퍼뜨리고 번쩍이게 한다. 0이면 유리 조각으로 깨뜨린다.
 * 비율이 남아 있으면 CrackSystem(피격 지점 이펙트), 0이면 BreakSystem(선택, 비워도 됨)을 생성한다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayCueNotify_ShieldHit : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	ULeeGameplayCueNotify_ShieldHit();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	/** 쉴드가 남아 있을 때 피격 지점 파편 이펙트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield")
	TObjectPtr<UNiagaraSystem> CrackSystem;

	/** 쉴드 HP 0 — 깨질 때 함께 낼 추가 이펙트 (유리 조각은 쉴드 Cue가 직접 만든다. 비워도 된다) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|Shield")
	TObjectPtr<UNiagaraSystem> BreakSystem;
};
