// Fill out your copyright notice in the Description page of Project Settings.


#include "LeeBossExpeditionSubsystem.h"

#include "CommonSessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GAS_Project/LeeLogChannels.h"
#include "GAS_Project/ASaveGame/LeeSaveSubsystem.h"

// 월드 컨텍스트에서 GameInstance의 Subsystem을 찾는다
ULeeBossExpeditionSubsystem* ULeeBossExpeditionSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<ULeeBossExpeditionSubsystem>() : nullptr;
}

// 연결 끊김 / 맵 로드 이벤트 구독
void ULeeBossExpeditionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::HandleNetworkFailure);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
}

// 구독 해제
void ULeeBossExpeditionSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
	}
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

	Super::Deinitialize();
}

// 출발한 필드맵 기억 + 원정 시작 표시
void ULeeBossExpeditionSubsystem::BeginExpedition(const UWorld* FieldWorld)
{
	if (FieldWorld)
	{
		FieldMapName = UWorld::RemovePIEPrefix(FieldWorld->GetOutermost()->GetName());
	}
	bInExpedition = true;
}

// 원정 표시만 해제 (필드에 그대로 남는 경우)
void ULeeBossExpeditionSubsystem::CancelExpedition()
{
	bInExpedition = false;
	bReturnAfterNextMapLoad = false;
}

// 세션을 정리하고 자기 필드맵을 연다 (리슨 옵션 없이 열어 혼자 플레이로 돌아간다)
void ULeeBossExpeditionSubsystem::ReturnToField()
{
	bInExpedition = false;
	bReturnAfterNextMapLoad = false;

	if (UCommonSessionSubsystem* SessionSubsystem = GetGameInstance()->GetSubsystem<UCommonSessionSubsystem>())
	{
		SessionSubsystem->CleanUpSessions();
	}

	const FString ReturnMapName = GetReturnMapName();
	if (ReturnMapName.IsEmpty())
	{
		UE_LOG(LogLee, Warning, TEXT("[BossExpedition] 돌아갈 필드맵이 없습니다 (세이브 기록·출발 맵 모두 비어 있음)."));
		return;
	}

	UGameplayStatics::OpenLevel(GetGameInstance(), FName(*ReturnMapName));
}

// 세이브에 기록된 마지막 휴식 맵이 우선, 없으면 원정을 시작한 맵
FString ULeeBossExpeditionSubsystem::GetReturnMapName() const
{
	if (const ULeeSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<ULeeSaveSubsystem>())
	{
		const FString SavedMapName = SaveSubsystem->GetSavedMapName();
		if (!SavedMapName.IsEmpty())
		{
			return SavedMapName;
		}
	}
	return FieldMapName;
}

// 원정 중 게임 연결(접속 중/접속 대기)이 끊긴 경우만 처리 — 비콘 연결 실패 등은 무시
void ULeeBossExpeditionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (!bInExpedition || !NetDriver)
	{
		return;
	}

	if (NetDriver->NetDriverName == NAME_GameNetDriver || NetDriver->NetDriverName == NAME_PendingNetDriver)
	{
		UE_LOG(LogLee, Log, TEXT("[BossExpedition] 원정 중 연결 끊김 (%s) → 기본 맵 로드 후 필드로 복귀"), *ErrorString);
		bReturnAfterNextMapLoad = true;
	}
}

// 연결이 끊겨 엔진이 기본 맵으로 보낸 경우, 로드가 끝나면 바로 자기 필드맵으로 다시 이동
void ULeeBossExpeditionSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (bReturnAfterNextMapLoad && LoadedWorld && LoadedWorld->GetGameInstance() == GetGameInstance())
	{
		ReturnToField();
	}
}
