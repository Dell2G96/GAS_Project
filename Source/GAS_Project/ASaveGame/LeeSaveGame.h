// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "LeeSaveGame.generated.h"

// 디스크에 저장되는 진행 데이터 묶음 (슬롯 1개, 자동 저장)
// 월드 상태(마지막 휴식 세이브 포인트, 처치한 정예 몹)는 SavedMapName과 같은 맵에서만 적용한다
UCLASS()
class GAS_PROJECT_API ULeeSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 아래 월드 상태가 기록된 맵 이름 (PIE 접두사 제외)
	UPROPERTY()
	FString SavedMapName;

	// 마지막으로 휴식한 세이브 포인트 ID (부활 위치)
	UPROPERTY()
	FName LastRestedSavePointId;

	// 처치되어 다시 생성되지 않는 스폰 지점 ID (bRespawnOnReset = false인 정예 몹)
	UPROPERTY()
	TSet<FName> DefeatedSpawnPointIds;

	// 에스트 잔여 횟수 (음수면 아직 저장된 적 없음 → 최대치 사용)
	UPROPERTY()
	float EstusCharges = -1.0f;
};
