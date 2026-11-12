// 26.11.12 신규 - 퍼펙트 가드 피드백 GameplayCue (GameplayCue.Souls.Guard.Perfect)

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "LeeGameplayCueNotify_PerfectGuard.generated.h"

class UCameraShakeBase;
class UNiagaraSystem;
class USoundBase;

// 퍼펙트 가드 순간 막은 지점에 불꽃 이펙트·금속음을 내고, 막은 플레이어의 카메라를 짧게 흔든다
// Location = 막은 지점, Normal = 공격이 들어온 방향. 모든 클라이언트에서 실행되며 카메라 흔들림은 막은 본인 화면에만
UCLASS()
class GAS_PROJECT_API ULeeGameplayCueNotify_PerfectGuard : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	ULeeGameplayCueNotify_PerfectGuard();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	/** 막은 지점에 생성할 이펙트 (비워도 됨) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|PerfectGuard")
	TObjectPtr<UNiagaraSystem> GuardSystem;

	/** 이펙트 크기 배율 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|PerfectGuard")
	FVector GuardSystemScale = FVector::OneVector;

	/** 막은 지점에서 재생할 사운드 (비워도 됨) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|PerfectGuard")
	TObjectPtr<USoundBase> GuardSound;

	/** 사운드 음높이 배율 (일반 피격음을 재사용할 때 높여서 '팅' 느낌을 낸다) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|PerfectGuard")
	float GuardSoundPitch = 1.0f;

	/** 막은 플레이어 화면에만 재생할 카메라 흔들림 (기본: ULeeCameraShake_PerfectGuard) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|PerfectGuard")
	TSubclassOf<UCameraShakeBase> CameraShake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|PerfectGuard")
	float CameraShakeScale = 1.0f;
};
