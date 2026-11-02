// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconClient.h"
#include "LeeBossJoinBeaconClient.generated.h"

// 입장 요청 결과 (수락 / 거절 / 연결 실패·방 사라짐)
UENUM()
enum class ELeeBossJoinResult : uint8
{
	Approved,
	Rejected,
	Failed
};

DECLARE_DELEGATE_OneParam(FOnLeeBossJoinResult, ELeeBossJoinResult);

// 26.11.02 추가 - 보스 방 "입장 요청" 비콘. 클라이언트는 필드맵에 있는 채로 호스트에 접속해 요청만 보내고,
// 호스트가 수락/거절하면 결과를 받는다 (수락 후에 실제 세션 참가). 호스트 쪽에도 같은 클래스가 생성되어 요청자 정보를 가진다
UCLASS(transient, notplaceable)
class GAS_PROJECT_API ALeeBossJoinBeaconClient : public AOnlineBeaconClient
{
	GENERATED_BODY()

public:
	// [클라이언트] 호스트 비콘 주소로 접속 시작. 접속되면 이름과 함께 입장 요청을 보낸다
	bool RequestJoin(const FString& ConnectInfo, const FString& InPlayerName);

	// [호스트] 요청한 플레이어 이름
	const FString& GetRequestPlayerName() const { return PlayerName; }

	// [호스트 → 클라이언트] 입장 수락
	UFUNCTION(Client, Reliable)
	void ClientJoinApproved();

	// [호스트 → 클라이언트] 입장 거절
	UFUNCTION(Client, Reliable)
	void ClientJoinRejected();

	// [클라이언트] 결과 알림 (위젯이 구독)
	FOnLeeBossJoinResult OnJoinResult;

protected:
	//~AOnlineBeaconClient interface
	virtual void OnConnected() override;
	virtual void OnFailure() override;
	//~End of AOnlineBeaconClient interface

	// [클라이언트 → 호스트] 입장 요청 (호스트 쪽 요청 목록에 추가됨)
	UFUNCTION(Server, Reliable)
	void ServerRequestJoin(const FString& InPlayerName);

private:
	// 결과를 한 번만 알리고, 네트워크 처리 도중 파괴되지 않도록 다음 틱에 비콘을 정리한다
	void FinishWithResult(ELeeBossJoinResult Result);

	FString PlayerName;

	bool bResultReceived = false;
};
