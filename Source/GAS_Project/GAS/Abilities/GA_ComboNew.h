// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CGameplayAbility.h"
#include "GA_ComboNew.generated.h"

/**
 * 
 */
UCLASS()
class GAS_PROJECT_API UGA_ComboNew : public UCGameplayAbility
{
	GENERATED_BODY()

	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	FName StartSocket = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	FName EndSocket = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|AttackTrace")
	float HitBoxRadius = 30.f;

	UFUNCTION(BlueprintCallable,  category="GAS|Combo")
	void SendHitReacEventToActors(const TArray<class AActor*>& HitActors);

	UFUNCTION(BlueprintCallable,  category="GAS|Combo")
	TArray<class AActor*> HitBoxTrace();

	UFUNCTION(BlueprintCallable,  category="GAS|Combo")
	void HitScanStart();

	UFUNCTION(BlueprintCallable,  category="GAS|Combo")
	void HitScanEnd();

	FTimerHandle HitBoxTraceTimerHandle;
	
	// 타이머에서 주기적으로 호출되는 함수
	UFUNCTION()
	void HitScanTick();

	
	void DrawDebugHitTrace(const TArray<FHitResult>& Hits, const FVector& HitBoxLocation) const;
};
