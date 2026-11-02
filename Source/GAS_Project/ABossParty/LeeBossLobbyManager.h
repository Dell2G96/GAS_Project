// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/OnlineReplStructs.h"
#include "LeeBossLobbyManager.generated.h"

class AController;
class AGameModeBase;
class ALeeBossJoinBeaconHostObject;
class APlayerController;
class APlayerState;
class UCommonActivatableWidget;

// 26.11.03 추가 - 입장을 수락했지만 아직 접속하지 않은 게스트 1명 (누가, 언제까지 와야 하는지)
struct FLeeBossArrival
{
	// 비콘 접속 때 받은 플레이어 고유 ID (게임 접속 후 PlayerState의 UniqueId와 비교한다)
	FUniqueNetIdRepl PlayerId;

	// 이 시각(서버 월드 시간)까지 접속하지 않으면 접속 실패로 본다
	float Deadline = 0.0f;
};

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

	// 26.11.03 추가 - 입장을 수락했지만 아직 접속을 끝내지 않은 인원 수 (클라이언트에도 복제)
	int32 GetArrivingPlayerCount() const { return ArrivingPlayerCount; }

	// [서버] 입장 요청을 관리하는 비콘 HostObject (호스트 HUD의 요청 목록용)
	ALeeBossJoinBeaconHostObject* GetJoinHostObject() const { return JoinHostObject; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 전원 준비 후 출발까지 카운트다운(초)
	UPROPERTY(EditAnywhere, Category = "Lee|BossLobby", meta = (ClampMin = 0))
	float CountdownSeconds = 5.0f;

	// 26.11.03 추가 - 수락한 게스트의 접속을 기다리는 최대 시간(초). 넘으면 접속 실패로 보고 출발을 다시 허용한다
	UPROPERTY(EditAnywhere, Category = "Lee|BossLobby", meta = (ClampMin = 1))
	float ArrivalTimeoutSeconds = 30.0f;

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

	// 26.11.03 추가 - [서버] 입장 수락 → 접속 대기 인원에 추가 (카운트다운 중이면 취소된다)
	void HandleJoinApproved(const FUniqueNetIdRepl& PlayerId);

	// 26.11.03 추가 - [서버] 기한이 지난 접속 대기 인원을 정리하고, 다음 기한에 맞춰 타이머를 다시 건다
	void RemoveExpiredArrivals();

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

	// 26.11.03 추가 - [서버] 접속 대기 인원 (수락한 순서대로 쌓이므로 0번의 기한이 가장 빠르다)
	TArray<FLeeBossArrival> PendingArrivals;

	// 26.11.03 추가 - PendingArrivals 개수 (HUD 표시용으로 모든 클라이언트에 복제)
	UPROPERTY(Replicated)
	int32 ArrivingPlayerCount = 0;

	UPROPERTY()
	TObjectPtr<ALeeBossJoinBeaconHostObject> JoinHostObject;

	TWeakObjectPtr<UCommonActivatableWidget> LobbyHUDWidget;

	FTimerHandle RegisterRetryTimerHandle;
	FTimerHandle ShowHUDRetryTimerHandle;
	FTimerHandle CountdownTimerHandle;
	FTimerHandle ArrivalTimeoutTimerHandle;

	FDelegateHandle PostLoginHandle;
	FDelegateHandle LogoutHandle;

	bool bTravelStarted = false;
};
