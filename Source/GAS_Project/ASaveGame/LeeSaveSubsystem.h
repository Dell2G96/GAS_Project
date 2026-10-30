// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LeeSaveSubsystem.generated.h"

class ULeeSaveGame;

// 세이브 데이터 담당 (GameInstance 수명 → 레벨이 바뀌어도 유지)
// 시작 시 슬롯에서 불러오고, 휴식/정예 처치/게임 종료 때 디스크에 저장한다. 자기 PC의 자기 진행만 저장
UCLASS()
class GAS_PROJECT_API ULeeSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End of USubsystem interface

	// 저장된 마지막 휴식 세이브 포인트 ID (다른 맵의 기록이면 None)
	FName GetLastRestedSavePointId(const UWorld* World) const;

	// 이 세이브 포인트가 점화된 적이 있는지 (다른 맵의 기록이면 false)
	bool IsSavePointLit(const UWorld* World, FName SavePointId) const;

	// 이 스폰 지점의 몹이 처치되어 다시 생성되면 안 되는지 (다른 맵의 기록이면 false)
	bool IsSpawnPointDefeated(const UWorld* World, FName SpawnPointId) const;

	// 게임 시작 후 첫 스폰에만 저장된 에스트 횟수를 돌려준다 (이후 호출이나 저장값이 없으면 false)
	bool ConsumeSavedEstusCharges(float& OutEstusCharges);

	// 휴식 기록(마지막 휴식 지점 + 점화 목록) 후 디스크에 저장
	void RecordRest(const UWorld* World, FName SavePointId);

	// 정예 몹 처치 기록 후 디스크에 저장
	void RecordSpawnPointDefeated(const UWorld* World, FName SpawnPointId);

	// 에스트 횟수를 메모리에만 갱신 (디스크 저장은 휴식/종료 때)
	void UpdateEstusCharges(float NewEstusCharges);

	// 현재 데이터를 슬롯에 저장
	void SaveToDisk();

private:
	// 저장된 월드 상태가 이 월드(맵)의 것인지
	bool IsSameMap(const UWorld* World) const;

	// 다른 맵의 월드 상태면 비우고 이 맵 이름으로 바꾼다 (월드 상태를 기록하기 전에 호출)
	void PrepareWorldState(const UWorld* World);

	// 현재 세이브 데이터 (Initialize에서 불러오거나 새로 생성)
	UPROPERTY()
	TObjectPtr<ULeeSaveGame> SaveGame;

	// 저장된 에스트 횟수를 이미 첫 스폰에 적용했는지
	bool bSavedEstusConsumed = false;
};
