// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "AHitGameplayCue.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API AHitGameplayCue : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()
	

public:
	AHitGameplayCue();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	// 피격 이펙트 지속 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Effect")
	float HitDuration = 0.8f;

	// 피격 강도 (머티리얼 파라미터 값)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Effect")
	float HitIntensity = 1.0f;

	// 피격 색상 (선택사항)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Effect")
	FName HitColorParameterName = "";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Effect")
	FLinearColor HitColor = FLinearColor::White;

	// 타이머 핸들
	FTimerHandle HitRecoveryTimerHandle;

private:
	// 피격 파라미터 변경
	void ApplyHitEffect(AActor* TargetActor);

	// 피격 파라미터 복원
	void RestoreHitEffect(AActor* TargetActor);
	
};
