// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "LeeBonfireSubsystem.generated.h"

struct FLeeVerbMessage;

// 화톳불/사망 관련 월드 규칙 담당 (서버 전용)
// 플레이어 사망 → YOU DIED 메시지 → 일정 시간 뒤 Enemy 리셋 + 플레이어 부활
UCLASS(Config = Game)
class GAS_PROJECT_API ULeeBonfireSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~UWorldSubsystem interface
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	//~End of UWorldSubsystem interface

	// 월드의 모든 Enemy(봇)를 처음 자리로 되돌린다. 살아있는 Enemy와 시체 모두 정리 후 재시작
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Lee|Bonfire")
	void ResetEnemies();

protected:
	//~USubsystem interface
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	//~End of USubsystem interface

private:
	// 사망 메시지(Lyra.Elimination.Message) 수신 → 플레이어면 YOU DIED 전송 + 부활 타이머 시작
	void HandleEliminationMessage(FGameplayTag Channel, const FLeeVerbMessage& Message);

	// 부활 타이머 만료 → 이전 Pawn 정리, Enemy 리셋, 플레이어 재시작
	void RespawnPlayer(TWeakObjectPtr<APlayerController> WeakPlayerController);

	// 해당 플레이어에게만 UI 메시지 전송 (로컬 플레이어는 직접, 원격 플레이어는 Client RPC)
	void SendMessageToPlayer(APlayerController* PlayerController, FGameplayTag MessageTag) const;

	// Pawn을 리셋용으로 정리한다 (ASC 연결 해제 + 파괴)
	static void DestroyPawnForReset(APawn* Pawn);

	// 사망 후 부활까지 대기 시간(초). DefaultGame.ini의 [/Script/GAS_Project.LeeBonfireSubsystem]에서 조절
	UPROPERTY(Config)
	float RespawnDelay = 4.0f;

	// 부활 대기 중인 플레이어 (중복 사망 메시지로 타이머가 두 번 걸리는 것 방지)
	TSet<TWeakObjectPtr<APlayerController>> PendingRespawnPlayers;

	FGameplayMessageListenerHandle EliminationListenerHandle;
};
