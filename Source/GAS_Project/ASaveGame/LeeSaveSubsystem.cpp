// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeSaveSubsystem.h"

#include "LeeSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "GAS_Project/LeeLogChannels.h"

namespace LeeSave
{
	// 세이브 슬롯은 1개만 사용한다
	static const FString SlotName = TEXT("LeeSaveSlot_0");
	static constexpr int32 UserIndex = 0;
}

// 슬롯에 저장된 데이터를 불러오고, 없으면 새 데이터를 만든다
void ULeeSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameplayStatics::DoesSaveGameExist(LeeSave::SlotName, LeeSave::UserIndex))
	{
		SaveGame = Cast<ULeeSaveGame>(UGameplayStatics::LoadGameFromSlot(LeeSave::SlotName, LeeSave::UserIndex));
	}

	if (!SaveGame)
	{
		SaveGame = Cast<ULeeSaveGame>(UGameplayStatics::CreateSaveGameObject(ULeeSaveGame::StaticClass()));
	}
}

// 게임 종료(PIE 종료 포함) 시 마지막 상태(에스트 횟수 등)를 저장한다
void ULeeSaveSubsystem::Deinitialize()
{
	SaveToDisk();

	Super::Deinitialize();
}

// 같은 맵의 기록일 때만 마지막 휴식 세이브 포인트 ID를 돌려준다
FName ULeeSaveSubsystem::GetLastRestedSavePointId(const UWorld* World) const
{
	return IsSameMap(World) ? SaveGame->LastRestedSavePointId : NAME_None;
}

// 같은 맵의 기록이고 점화 목록에 있으면 true
bool ULeeSaveSubsystem::IsSavePointLit(const UWorld* World, FName SavePointId) const
{
	return IsSameMap(World) && !SavePointId.IsNone() && SaveGame->LitSavePointIds.Contains(SavePointId);
}

// 같은 맵의 기록이고 처치 목록에 있으면 true
bool ULeeSaveSubsystem::IsSpawnPointDefeated(const UWorld* World, FName SpawnPointId) const
{
	return IsSameMap(World) && !SpawnPointId.IsNone() && SaveGame->DefeatedSpawnPointIds.Contains(SpawnPointId);
}

// 게임 실행 중 한 번만 저장된 에스트 횟수를 꺼내준다 (사망 부활 때는 기존처럼 최대치 충전)
bool ULeeSaveSubsystem::ConsumeSavedEstusCharges(float& OutEstusCharges)
{
	if (bSavedEstusConsumed || !SaveGame || SaveGame->EstusCharges < 0.0f)
	{
		return false;
	}

	bSavedEstusConsumed = true;
	OutEstusCharges = SaveGame->EstusCharges;
	return true;
}

// 마지막 휴식 세이브 포인트와 점화 목록을 기록하고 바로 저장한다 (휴식한 곳은 항상 점화된 상태)
void ULeeSaveSubsystem::RecordRest(const UWorld* World, FName SavePointId)
{
	PrepareWorldState(World);
	SaveGame->LastRestedSavePointId = SavePointId;
	SaveGame->LitSavePointIds.Add(SavePointId);
	SaveToDisk();
}

// 처치한 정예 몹의 스폰 지점을 기록하고 바로 저장한다
void ULeeSaveSubsystem::RecordSpawnPointDefeated(const UWorld* World, FName SpawnPointId)
{
	if (SpawnPointId.IsNone())
	{
		return;
	}

	PrepareWorldState(World);
	SaveGame->DefeatedSpawnPointIds.Add(SpawnPointId);
	SaveToDisk();
}

// 에스트 횟수를 메모리에만 기록한다 (게임 종료 시 Deinitialize에서 디스크에 저장됨)
void ULeeSaveSubsystem::UpdateEstusCharges(float NewEstusCharges)
{
	if (SaveGame)
	{
		SaveGame->EstusCharges = NewEstusCharges;
	}
}

// 현재 데이터를 슬롯에 저장한다
void ULeeSaveSubsystem::SaveToDisk()
{
	if (SaveGame && !UGameplayStatics::SaveGameToSlot(SaveGame, LeeSave::SlotName, LeeSave::UserIndex))
	{
		UE_LOG(LogLee, Warning, TEXT("[SaveSubsystem] 슬롯 [%s] 저장 실패"), *LeeSave::SlotName);
	}
}

// 저장된 맵 이름과 현재 맵 이름(PIE 접두사 제외)이 같은지 비교한다
bool ULeeSaveSubsystem::IsSameMap(const UWorld* World) const
{
	return SaveGame && World && SaveGame->SavedMapName == UGameplayStatics::GetCurrentLevelName(World, true);
}

// 다른 맵에서 처음 기록하는 경우 이전 맵의 월드 상태를 비운다
void ULeeSaveSubsystem::PrepareWorldState(const UWorld* World)
{
	if (!SaveGame || !World || IsSameMap(World))
	{
		return;
	}

	SaveGame->SavedMapName = UGameplayStatics::GetCurrentLevelName(World, true);
	SaveGame->LastRestedSavePointId = NAME_None;
	SaveGame->LitSavePointIds.Empty();
	SaveGame->DefeatedSpawnPointIds.Empty();
}
