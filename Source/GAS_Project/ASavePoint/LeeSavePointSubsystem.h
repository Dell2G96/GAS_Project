// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "LeeSavePointSubsystem.generated.h"

struct FLeeVerbMessage;
class ALeeSavePointActor;
class ALeePlayerStart;

// 세이브 포인트/사망 관련 월드 규칙 담당 (서버 전용)
// 플레이어 사망 → YOU DIED 메시지 → 플레이어가 '예'로 부활 요청 → Enemy 리셋 + 플레이어 부활
UCLASS()
class GAS_PROJECT_API ULeeSavePointSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~UWorldSubsystem interface
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	//~End of UWorldSubsystem interface

	// 월드의 모든 Enemy(봇)를 처음 자리로 되돌린다. 살아있는 Enemy와 시체 모두 정리 후 재시작
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Lee|SavePoint")
	void ResetEnemies();

	// 사망 대기 중인 플레이어의 부활 요청 처리 (ALeePlayerController::ServerRequestRespawn에서 호출)
	void RequestRespawn(APlayerController* PlayerController);

	// 세이브 포인트 등록/해제 (ALeeSavePointActor::BeginPlay/EndPlay에서 호출)
	void RegisterSavePoint(ALeeSavePointActor* SavePoint);
	void UnregisterSavePoint(ALeeSavePointActor* SavePoint);

	// SavePointId로 세이브 포인트 조회 (없으면 nullptr)
	ALeeSavePointActor* FindSavePointById(FName SavePointId) const;

	// 어떤 세이브 포인트의 부활 지점으로 지정된 PlayerStart인지 (일반 스폰 후보에서 제외할 때 사용)
	bool IsSavePointRespawnStart(const ALeePlayerStart* PlayerStart) const;

protected:
	//~USubsystem interface
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	//~End of USubsystem interface

private:
	// 사망 메시지(Lyra.Elimination.Message) 수신 → 플레이어면 YOU DIED 전송 + 부활 대기 등록
	void HandleEliminationMessage(FGameplayTag Channel, const FLeeVerbMessage& Message);

	// 이전 Pawn 정리, Enemy 리셋, 플레이어 재시작
	void RespawnPlayer(APlayerController* PlayerController);

	// 해당 플레이어에게만 UI 메시지 전송 (로컬 플레이어는 직접, 원격 플레이어는 Client RPC)
	void SendMessageToPlayer(APlayerController* PlayerController, FGameplayTag MessageTag) const;

	// Pawn을 리셋용으로 정리한다 (ASC 연결 해제 + 파괴)
	static void DestroyPawnForReset(APawn* Pawn);

	// 부활 대기 중인 플레이어 (죽은 플레이어만 부활 요청 가능, 중복 요청 방지)
	TSet<TWeakObjectPtr<APlayerController>> PendingRespawnPlayers;

	FGameplayMessageListenerHandle EliminationListenerHandle;

	// 등록된 세이브 포인트 (SavePointId → 세이브 포인트)
	TMap<FName, TWeakObjectPtr<ALeeSavePointActor>> SavePoints;
};
