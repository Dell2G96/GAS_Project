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

protected:
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
