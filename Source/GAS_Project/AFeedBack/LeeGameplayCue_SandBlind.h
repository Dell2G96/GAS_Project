#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "LeeGameplayCue_SandBlind.generated.h"

class UCameraComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * 모래 시야 방해 GameplayCue (태그: GameplayCue.Souls.SandBlind).
 * GE_SandBlind가 걸려 있는 동안, 맞은 플레이어 본인 화면에만 모래 포스트프로세스 머티리얼을 덮는다.
 * WhileActive → 페이드 인, OnRemove → 페이드 아웃 후 머티리얼 제거. 클라이언트 전용 연출.
 */
UCLASS()
class GAS_PROJECT_API ALeeGameplayCue_SandBlind : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	ALeeGameplayCue_SandBlind();

	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool Recycle() override;

protected:
	/** 화면에 덮을 포스트프로세스 머티리얼 (BP에서 M_PP_SandBlind 지정) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|SandBlind")
	TObjectPtr<UMaterialInterface> SandMaterial;

	/** 머티리얼의 강도 스칼라 파라미터 이름 (0 = 없음, 1 = 최대) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|SandBlind")
	FName AmountParamName = TEXT("SandAmount");

	/** 모래가 덮이는 시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|SandBlind", meta = (ClampMin = "0.01"))
	float FadeInTime = 0.2f;

	/** GE가 끝난 뒤 모래가 걷히는 시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lee|SandBlind", meta = (ClampMin = "0.01"))
	float FadeOutTime = 1.0f;

private:
	/** 페이드 목표값을 정하고 Tick을 켠다 */
	void StartFade(float InTargetAmount, float InFadeTime);

	/** 카메라에서 머티리얼을 떼고 상태 초기화 (페이드 아웃 완료/EndPlay/Recycle 공용) */
	void ClearEffect();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SandMID;

	TWeakObjectPtr<UCameraComponent> TargetCamera;

	float CurrentAmount = 0.f;
	float TargetAmount = 0.f;
	float FadeSpeed = 1.f;
};
