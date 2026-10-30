// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "LeeEnemySpawnSubsystem.generated.h"

struct FLeeVerbMessage;
class ALeeEnemySpawnPoint;
class ULeeExperienceDefinition;

// Enemy 자동 생성 담당 (서버 전용, 에셋 설정 불필요)
// Experience 로드가 끝나면 레벨의 모든 ALeeEnemySpawnPoint마다 몹을 1마리씩 생성한다
// 26.10.30 추가 - 정예 몹(bRespawnOnReset = false) 처치를 세이브에 기록하고, 처치된 자리는 생성하지 않는다
UCLASS()
class GAS_PROJECT_API ULeeEnemySpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~UWorldSubsystem interface
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	//~End of UWorldSubsystem interface

	// 26.10.30 추가 - 이 스폰 지점이 처치된 정예 몹 자리라서 생성하면 안 되는지 (세이브 기준)
	bool IsSpawnPointDefeated(const ALeeEnemySpawnPoint* SpawnPoint) const;

protected:
	//~USubsystem interface
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	//~End of USubsystem interface

private:
	// Experience 로드 완료 → 모든 스폰 지점에 몹 생성
	void OnExperienceLoaded(const ULeeExperienceDefinition* Experience);

	// 스폰 지점 하나에 봇 Controller를 만들고 그 자리에서 몹을 시작시킨다
	void SpawnEnemy(ALeeEnemySpawnPoint* SpawnPoint);

	// 26.10.30 추가 - 사망 메시지 수신 → 정예 몹이면 스폰 지점 ID를 세이브에 기록
	void HandleEliminationMessage(FGameplayTag Channel, const FLeeVerbMessage& Message);

	// 26.10.30 추가 - SpawnPointId가 비었거나(정예 몹) 겹치는 스폰 지점을 경고한다
	void ValidateSpawnPointIds() const;

	FGameplayMessageListenerHandle EliminationListenerHandle;
};
