// 26.11.11 신규 - 공용 피격 이펙트·사운드 GameplayCue (GameplayCue.Souls.Hit.Impact)

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "LeeGameplayCueNotify_HitImpact.generated.h"

class UNiagaraSystem;
class USoundBase;

/**
 * 몸에 맞은 모든 피격(일반 피격·슈퍼아머 피격)마다 실행되는 Static Cue.
 * Location = 맞은 지점, Normal = 공격이 들어온 방향. 그 위치에 나이아가라와 사운드를 재생한다.
 */
UCLASS()
class GAS_PROJECT_API ULeeGameplayCueNotify_HitImpact : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	ULeeGameplayCueNotify_HitImpact();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	/** 맞은 지점에 생성할 이펙트 (비워도 됨) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|HitImpact")
	TObjectPtr<UNiagaraSystem> ImpactSystem;

	/** 26.11.11 추가 - 피격 이펙트 크기 배율 (1 = 원본 크기) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|HitImpact")
	FVector ImpactScale = FVector::OneVector;

	/** 맞은 지점에서 재생할 사운드 (비워도 됨) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|HitImpact")
	TObjectPtr<USoundBase> ImpactSound;
};
