// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHostObject.h"
#include "GameFramework/OnlineReplStructs.h"
#include "LeeBossJoinBeaconHostObject.generated.h"

class ALeeBossJoinBeaconClient;

DECLARE_MULTICAST_DELEGATE(FOnLeeBossJoinRequestsChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLeeBossJoinRequestApproved, const FUniqueNetIdRepl& /*PlayerId*/);

// 26.11.02 추가 - [호스트] 입장 요청 비콘의 서버 측 관리자. 세션 비콘 리스너(같은 포트)에 등록되어
// 들어온 입장 요청을 대기 목록으로 보관하고, 로비 HUD의 수락/거절 버튼에 따라 결과를 돌려준다
UCLASS(transient, notplaceable)
class GAS_PROJECT_API ALeeBossJoinBeaconHostObject : public AOnlineBeaconHostObject
{
	GENERATED_BODY()

public:
	ALeeBossJoinBeaconHostObject(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 새 입장 요청을 대기 목록에 추가
	void AddRequest(ALeeBossJoinBeaconClient* Request);

	// 수락 → 요청자에게 알리고 목록에서 제거 (요청자는 이후 세션에 참가)
	void ApproveRequest(ALeeBossJoinBeaconClient* Request);

	// 거절 → 요청자에게 알리고 목록에서 제거
	void RejectRequest(ALeeBossJoinBeaconClient* Request);

	// 대기 중인 요청 모두 거절 (보스맵 출발 시)
	void RejectAllRequests();

	const TArray<TObjectPtr<ALeeBossJoinBeaconClient>>& GetPendingRequests() const { return PendingRequests; }

	// 대기 목록이 바뀔 때 (로비 HUD가 구독해 목록을 다시 그린다)
	FOnLeeBossJoinRequestsChanged OnRequestsChanged;

	// 26.11.03 추가 - 요청을 수락했을 때 요청자의 고유 ID와 함께 알림 (로비 관리 Actor가 "접속 대기 인원"으로 기록해 출발을 막는다)
	FOnLeeBossJoinRequestApproved OnRequestApproved;

	//~AOnlineBeaconHostObject interface
	virtual void NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor) override;
	//~End of AOnlineBeaconHostObject interface

private:
	UPROPERTY()
	TArray<TObjectPtr<ALeeBossJoinBeaconClient>> PendingRequests;
};
