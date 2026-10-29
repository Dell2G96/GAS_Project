// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LeeEnemySpawnSubsystem.generated.h"

class ALeeEnemySpawnPoint;
class ULeeExperienceDefinition;

// Enemy 자동 생성 담당 (서버 전용, 에셋 설정 불필요)
// Experience 로드가 끝나면 레벨의 모든 ALeeEnemySpawnPoint마다 몹을 1마리씩 생성한다
UCLASS()
class GAS_PROJECT_API ULeeEnemySpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~UWorldSubsystem interface
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	//~End of UWorldSubsystem interface

protected:
	//~USubsystem interface
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	//~End of USubsystem interface

private:
	// Experience 로드 완료 → 모든 스폰 지점에 몹 생성
	void OnExperienceLoaded(const ULeeExperienceDefinition* Experience);

	// 스폰 지점 하나에 봇 Controller를 만들고 그 자리에서 몹을 시작시킨다
	void SpawnEnemy(ALeeEnemySpawnPoint* SpawnPoint);
};
