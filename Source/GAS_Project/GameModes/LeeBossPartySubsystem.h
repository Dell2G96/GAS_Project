// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "LeeBossPartySubsystem.generated.h"

class APlayerController;
class ULeeExperienceDefinition;

// 26.11.02 추가 - 보스전(MultiplayerBoss) 파티 상태를 서버에서 관리한다.
// 넉다운 가능 여부 판정, 사망 시 관전 대상 지정, 전멸 시 YOU DIED 후 로비 이동을 담당
UCLASS()
class GAS_PROJECT_API ULeeBossPartySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	// 현재 Experience가 보스전(MultiplayerBoss)인지
	static bool IsBossMultiplayer(const UObject* WorldContextObject);

	// HP가 0이 된 플레이어가 넉다운으로 버틸 수 있는지 (보스전 + 아직 서 있는 동료가 1명 이상)
	bool CanEnterDowned(const APawn* VictimPawn) const;

	// 플레이어 사망 시작 시 호출 (서버). 전멸이면 로비 이동, 아니면 관전 시작
	void HandlePlayerDeathStarted(APawn* DeadPawn);

	// 26.11.02 추가 - Actor 사망 시작 시 호출 (서버). Actor Tag "Boss"가 있으면 보스 처치 → VICTORY 후 각자 필드로 복귀
	void HandleBossDeathStarted(AActor* DeadActor);

	// 관전 중인 플레이어의 카메라를 다음 생존 동료로 전환 (서버)
	void SpectateNextTeammate(APlayerController* SpectatorController);

	// 넉다운도 사망도 아닌 플레이어인지
	static bool IsPlayerStanding(const APawn* Pawn);

	// 사망하지 않은 플레이어인지 (넉다운 포함) — 관전 대상 판정용
	static bool IsPlayerNotDead(const APawn* Pawn);

private:
	const ULeeExperienceDefinition* GetBossExperience() const;

	// 조건에 맞는 플레이어 Pawn 목록 (bStandingOnly=true면 넉다운 제외)
	TArray<APawn*> GetPlayerPawns(bool bStandingOnly) const;

	// 관전 대상(사망하지 않은 동료) 중 Current 다음 Pawn을 고른다
	APawn* FindNextSpectateTarget(const APawn* Exclude, const AActor* Current) const;

	// 전멸 처리: 넉다운 중인 플레이어도 사망시키고, YOU DIED 후 로비로 이동 예약
	void StartPartyWipe();

	// 로비 맵으로 ServerTravel
	void TravelToLobby();

	// 다른 플레이어를 보고 있던 관전자를 새 대상으로 옮긴다
	void RetargetSpectatorsWatching(const APawn* LostTarget);

	// 26.11.02 추가 - 모든 플레이어에게 UI 메시지 전송 (호스트는 바로 방송, 원격은 PlayerState Client RPC)
	void SendMessageToAllPlayers(FGameplayTag MessageTag);

	// 26.11.02 추가 - 원격 플레이어에게 먼저 필드 복귀를 지시하고, 잠시 뒤 호스트도 복귀
	void ReturnPartyToField();

	// 26.11.02 추가 - 호스트(로컬 플레이어) 필드 복귀
	void ReturnHostToField();

	bool bWipeInProgress = false;

	// 26.11.02 추가 - 보스 처치 후 복귀 대기 중 (전멸 판정·넉다운 중단)
	bool bVictoryInProgress = false;

	FTimerHandle WipeTravelTimerHandle;

	FTimerHandle VictoryTravelTimerHandle;
};
