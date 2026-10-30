// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LeeEnemySpawnPoint.generated.h"

class ALeePlayerBotController;
class UArrowComponent;
class UCapsuleComponent;

// 레벨에 배치하는 Enemy 스폰 지점 — 이 자리마다 BotControllerClass 몹이 1마리 생성된다
// 리셋(휴식/사망) 후에도 같은 자리에서 다시 생성된다. 생성은 ULeeEnemySpawnSubsystem이 담당
UCLASS(Blueprintable)
class GAS_PROJECT_API ALeeEnemySpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ALeeEnemySpawnPoint();

	TSubclassOf<ALeePlayerBotController> GetBotControllerClass() const { return BotControllerClass; }
	const FText& GetDisplayNameOverride() const { return DisplayNameOverride; }
	FName GetSpawnPointId() const { return SpawnPointId; }
	bool ShouldRespawnOnReset() const { return bRespawnOnReset; }

protected:
	// 26.10.30 추가 - 스폰 지점 고유 ID (레벨 안에서 겹치면 안 됨). 세이브에서 "처치한 정예 몹" 기록에 사용
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lee|EnemySpawn")
	FName SpawnPointId;

	// 26.10.30 추가 - false면 정예 몹: 한 번 처치하면 휴식/사망 리셋 후에도 다시 생성되지 않는다 (SpawnPointId 필수)
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Lee|EnemySpawn")
	bool bRespawnOnReset = true;

	// 생성할 몹의 AI Controller BP (몹의 PawnData는 Controller BP의 EnemyPawnData에서 정함)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|EnemySpawn")
	TSubclassOf<ALeePlayerBotController> BotControllerClass;

	// 이 자리 몹만 다른 이름으로 표시 (정예/보스 등). 비우면 Controller BP의 EnemyDisplayName 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lee|EnemySpawn")
	FText DisplayNameOverride;

	// 에디터 표시용 캡슐 (게임에서는 숨김, 충돌 없음). 캡슐 중심이 몹이 스폰되는 위치
	UPROPERTY(VisibleAnywhere, Category = "Lee|EnemySpawn")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

#if WITH_EDITORONLY_DATA
	// 에디터 표시용 방향 화살표 (몹이 바라볼 방향)
	UPROPERTY()
	TObjectPtr<UArrowComponent> ArrowComponent;
#endif
};
