// Fill out your copyright notice in the Description page of Project Settings.
// 26.11.25 신규 작성 — 다른 Enemy가 "가는 중인 목적지"를 EQS에 넘겨 같은 지점으로 몰리지 않게 하는 컨텍스트

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "LeeEnvQueryContext_OtherEnemies.generated.h"

/**
 * 자기 자신을 뺀 살아 있는 다른 Enemy들의 "위치 목록"을 EQS에 넘기는 컨텍스트.
 * 이동 중인 Enemy는 목적지(경로 끝), 서 있거나 액터를 쫓는 Enemy는 현재 위치를 넘긴다.
 */
UCLASS(MinimalAPI)
class ULeeEnvQueryContext_OtherEnemies : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;

	/** Self를 뺀 살아 있는 다른 Enemy들의 목적지 또는 현재 위치를 모은다 (Strafe Task도 같은 규칙으로 쓴다) */
	static void GatherOtherEnemyLocations(const AActor* Self, TArray<FVector>& OutLocations);
};
