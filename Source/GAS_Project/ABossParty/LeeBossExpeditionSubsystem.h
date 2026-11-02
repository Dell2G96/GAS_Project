// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LeeBossExpeditionSubsystem.generated.h"

class UNetDriver;

// 26.11.02 추가 - 보스 원정(로비·보스맵 멀티) 중인지 기억하고, 끝나면 "자기 필드맵(마지막 세이브 포인트)"으로 돌려보낸다
// 로비 나가기 / 보스 처치 후 / 호스트 이탈(연결 끊김) 모두 이 Subsystem의 ReturnToField로 복귀한다 (GameInstance 수명 → 맵 이동에도 유지)
UCLASS()
class GAS_PROJECT_API ULeeBossExpeditionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static ULeeBossExpeditionSubsystem* Get(const UObject* WorldContextObject);

	//~USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End of USubsystem interface

	// 방 만들기 / 입장 직전에 호출 — 출발한 필드맵을 기억하고 원정 중으로 표시
	void BeginExpedition(const UWorld* FieldWorld);

	// 방 만들기 / 입장이 실패해 필드에 남았을 때 원정 표시 해제
	void CancelExpedition();

	// 세션 정리 후 자기 필드맵으로 이동 (세이브 맵 우선, 없으면 출발한 맵)
	void ReturnToField();

	bool IsInExpedition() const { return bInExpedition; }

private:
	// 돌아갈 맵 이름 (세이브의 마지막 휴식 맵 → 출발한 필드맵 순)
	FString GetReturnMapName() const;

	// 원정 중 게임 연결이 끊김(호스트 이탈 등) → 엔진이 기본 맵으로 보낸 뒤 필드로 다시 이동하도록 예약
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	// 맵 로드 완료 — 복귀 예약이 있으면 필드맵으로 이동
	void HandlePostLoadMap(UWorld* LoadedWorld);

	// 원정을 시작한 필드맵 (PIE 접두사 제외 패키지 경로)
	FString FieldMapName;

	bool bInExpedition = false;

	// 연결이 끊겨 다음 맵 로드 후 필드로 이동해야 하는지
	bool bReturnAfterNextMapLoad = false;

	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle PostLoadMapHandle;
};
