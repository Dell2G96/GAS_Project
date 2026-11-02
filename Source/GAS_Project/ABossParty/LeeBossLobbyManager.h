// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LeeBossLobbyManager.generated.h"

class AController;
class AGameModeBase;
class ALeeBossJoinBeaconHostObject;
class APlayerController;
class APlayerState;
class UCommonActivatableWidget;

// 26.11.02 추가 - 보스 로비 맵(L_BossLobby)에 1개 배치하는 관리 Actor
// [서버] 입장 요청 비콘 등록, 준비 상태 관리, 전원 준비 시 카운트다운 → 보스맵 이동 / [클라이언트] 로비 HUD 표시
UCLASS()
class GAS_PROJECT_API ALeeBossLobbyManager : public AActor
{
	GENERATED_BODY()

public:
	ALeeBossLobbyManager();

	// 현재 월드에 배치된 로비 관리 Actor (없으면 nullptr)
	static ALeeBossLobbyManager* Get(const UObject* WorldContextObject);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// [서버] 플레이어 준비 / 준비 취소
	void SetPlayerReady(APlayerState* PlayerState, bool bReady);

	bool IsPlayerReady(const APlayerState* PlayerState) const;

	// 출발까지 남은 시간(초). 카운트다운 중이 아니면 -1
	float GetCountdownRemaining() const;

	// [서버] 입장 요청을 관리하는 비콘 HostObject (호스트 HUD의 요청 목록용)
	ALeeBossJoinBeaconHostObject* GetJoinHostObject() const { return JoinHostObject; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 전원 준비 후 출발까지 카운트다운(초)
	UPROPERTY(EditAnywhere, Category = "Lee|BossLobby", meta = (ClampMin = 0))
	float CountdownSeconds = 5.0f;

	// 로비에서 띄울 HUD (ULeeBossLobbyHUDWidget 기반 BP, Input Config = GameAndMenu 권장)
	UPROPERTY(EditAnywhere, Category = "Lee|BossLobby")
	TSoftClassPtr<UCommonActivatableWidget> LobbyHUDClass;

private:
	// [서버] 세션의 비콘 리스너에 입장 요청 HostObject 등록 (리스너가 아직 없으면 잠시 후 재시도)
	void TryRegisterJoinHost();

	// [클라이언트/호스트] 로컬 플레이어 화면에 로비 HUD Push (UI가 준비 전이면 잠시 후 재시도)
	void TryShowLobbyHUD();

	// [서버] 접속/퇴장 시 준비 목록 정리 + 카운트다운 재판정
	void HandlePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);
	void HandleLogout(AGameModeBase* GameMode, AController* Exiting);

	// [서버] 전원 준비면 카운트다운 시작, 아니면 취소
	void EvaluateCountdown();

	// [서버] 카운트다운 종료 → 대기 요청 거절 후 Experience의 BossMap으로 전원 이동
	void StartBossTravel();

	// 준비 완료한 플레이어 목록 (서버 → 모든 클라이언트 복제)
	UPROPERTY(Replicated)
	TArray<TObjectPtr<APlayerState>> ReadyPlayers;

	// 출발 시각 (서버 월드 시간 기준, 0이면 카운트다운 중 아님)
	UPROPERTY(Replicated)
	float CountdownEndTime = 0.0f;

	UPROPERTY()
	TObjectPtr<ALeeBossJoinBeaconHostObject> JoinHostObject;

	TWeakObjectPtr<UCommonActivatableWidget> LobbyHUDWidget;

	FTimerHandle RegisterRetryTimerHandle;
	FTimerHandle ShowHUDRetryTimerHandle;
	FTimerHandle CountdownTimerHandle;

	FDelegateHandle PostLoginHandle;
	FDelegateHandle LogoutHandle;

	bool bTravelStarted = false;
};
